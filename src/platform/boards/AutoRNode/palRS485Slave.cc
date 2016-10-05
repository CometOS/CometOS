/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * CometOS Platform Abstraction Layer for RS485 Slave.
 *
 * @author Florian Meier
 */

#include "palRS485Slave.h"
#include <avr/interrupt.h>
#include "palExec.h"
#include "palRS485.h"

#include <stddef.h>

namespace cometos {

#define POLLFWDCMDNR 192
#define ALTPOLLFWDCMDNR 200
#define RS485MASTERADDR 3

class PalRS485SlaveImpl;

class PalRS485SlaveImpl : public PalRS485Slave {
private:
	bool initialized;

	uint8_t rxPos;
	uint8_t maxRxLength;
	palRS485Slave_rxPtr cbrx;
	uint8_t * rxBuffer;

	palRS485Slave_rxPtr cbrx_back;
	uint8_t * rxBuffer_back;
	uint8_t fwdLength_back;

	uint8_t txLength;
	palRS485Slave_txPtr cbtx;
	const uint8_t* txBuffer;
	uint8_t txCmdNumber;
    uint8_t txStatus;

	uint8_t address;
	uint8_t fwdLength;
	uint8_t rx_checksum;
	uint8_t rxCmdNumber;
	uint8_t rxType;

	enum class MsgType : uint8_t {
		POLL = 0,
		FWD = 1,
		OTHER = 0xFF
	};

	MsgType currType;

	enum class Status : uint8_t {
		OK = 100,
		OK_AGAIN = 102,
		NO_CMD = 64,
		MSG = 103,
		MSG_AGAIN = 104
	};

	Callback<bool()> dataPending;
	IGPIOPin* interruptPin;

public:
	virtual cometos_error_t init(uint8_t adr, Callback<bool()> dataPending, IGPIOPin* interruptPin);
	virtual cometos_error_t restart();
	virtual cometos_error_t depositReceptionBuffer(uint8_t* rxBuf, uint8_t maxLen, palRS485Slave_rxPtr callback);
	virtual cometos_error_t depositTransmissionData(const uint8_t* txBuf, uint8_t len, palRS485Slave_txPtr callback, uint8_t cmdNumber = 0xFF, uint8_t status = 0xFF);

	static PalRS485SlaveImpl rs485;

	static PalRS485SlaveImpl& getInstance() {
		// Instantiate class
		return rs485;
	}

private:
	PalRS485SlaveImpl();

	bool rxByteCallback(uint8_t value, bool flag);
	void tx();

private:
	BoundedTask<PalRS485SlaveImpl, &PalRS485SlaveImpl::tx> taskTx;
};

PalRS485SlaveImpl PalRS485SlaveImpl::rs485;

PalRS485SlaveImpl::PalRS485SlaveImpl()
: initialized(false), rxPos(0), maxRxLength(0), cbrx(), rxBuffer(NULL), cbrx_back(), rxBuffer_back(NULL), fwdLength_back(0), 
txLength(0), cbtx(), txBuffer(NULL), taskTx(*this)
{}

cometos_error_t PalRS485SlaveImpl::init(uint8_t adr, Callback<bool()> dataPending, IGPIOPin* interruptPin) {
	if (initialized) {
		return COMETOS_SUCCESS;
	}

	address = adr;
	this->dataPending = dataPending;
	this->interruptPin = interruptPin;
        this->interruptPin->set(); // high means no data available
        this->interruptPin->setDirection(IGPIOPin::Direction::OUT);

    restart();

	initialized = true;

	return COMETOS_SUCCESS;
}

cometos_error_t PalRS485SlaveImpl::restart() {
    palRS485_init(RS485_BAUDRATE,NULL,NULL,NULL);
    palRS485_9bit_mode(true,true,CALLBACK_MET(&PalRS485SlaveImpl::rxByteCallback, *this));
    return COMETOS_SUCCESS;
}

cometos_error_t PalRS485SlaveImpl::depositReceptionBuffer(uint8_t * rxBuf, uint8_t maxLen,
		palRS485Slave_rxPtr callback) {
	palExec_atomicBegin();
	bool busy = (rxBuffer != NULL || cbrx);
	if (busy) {
		palExec_atomicEnd();
		return COMETOS_ERROR_BUSY;
	}

	if (rxBuf == NULL || !callback) {
		palExec_atomicEnd();
		return COMETOS_ERROR_INVALID;
	}

	cbrx = callback;
	rxBuffer = rxBuf;
	maxRxLength = maxLen;

	palExec_atomicEnd();

	return COMETOS_SUCCESS;
}

cometos_error_t PalRS485SlaveImpl::depositTransmissionData(const uint8_t* txBuf, uint8_t len, palRS485Slave_txPtr callback, uint8_t cmdNumber, uint8_t status) {
	palExec_atomicBegin();
	bool busy = (txBuffer != NULL || cbtx);
	if (busy) {
		palExec_atomicEnd();
		return COMETOS_ERROR_BUSY;
	}

	if (txBuf == NULL) {
		palExec_atomicEnd();
		return COMETOS_ERROR_INVALID;
	}

	cbtx = callback;
	txBuffer = txBuf;
	txLength = len;
	txCmdNumber = cmdNumber;
	txStatus = status;
	this->interruptPin->clear();

	palExec_atomicEnd();

	return COMETOS_SUCCESS;
}

bool PalRS485SlaveImpl::rxByteCallback(uint8_t value, bool flag) {
	if(rxPos == 0 && !flag) {
		cometos::getCout() << "ERROR: Out of sync! rxPos == 0, but flag not set!" << cometos::endl;
		return false;
	}

	if(flag) {
		/* TODO Flush previous message if garbage? */

		rxPos = 1;
		rx_checksum = address;

		if(value != address) {
			// not for me
			return false;
		}
		else {
			// for me
			return true;
		}
	}
	else {
		if(rxPos <= 3) {
			rx_checksum += value;
		}

		if(rxPos == 1) {
			// Command Number
			rxCmdNumber = value;
		}
		else if(rxPos == 2) {
            rxType = value;
			if(rxCmdNumber == POLLFWDCMDNR && value == (uint8_t)MsgType::POLL) {
				currType = MsgType::POLL;
			}	
			else if(rxCmdNumber == POLLFWDCMDNR && value == (uint8_t)MsgType::FWD) {
				currType = MsgType::FWD;
			}	
			else {
			    currType = MsgType::OTHER;
			}
		}
		else if(rxPos == 3) {
			if(currType == MsgType::POLL && value != 0) {
				cometos::getCout() << "ERROR: POLL type, but length != 0" << cometos::endl;
				return false;
			}
			else {
				fwdLength = value;
			}
		}
		else if(rxPos == 4) {
			if(rx_checksum != value) {
				cometos::getCout() << "ERROR: Invalid checksum! Should be " << (int)rx_checksum << " is " << (int)value << cometos::endl;
				return false;
			}

			// now the header is finished
			if(currType == MsgType::POLL) {
				//cometos::getCout() << "Poll received" << cometos::endl;
				cometos::getScheduler().replace(taskTx);
			}

            if(currType == MsgType::OTHER) {
				cometos::getCout() << "Reply to OTHER" << cometos::endl;
				cometos::getScheduler().replace(taskTx);
            }
		}
		else {
			if(currType != MsgType::FWD) {
				cometos::getCout() << "ERROR: Encapsulated command, but not FWD " << (int)value << cometos::endl;
				return false;
			}
			else {
				uint8_t bufferPos = rxPos-5;

				if(bufferPos >= fwdLength) {
					cometos::getCout() << "ERROR: More bytes encapsulated as given by the length" << cometos::endl;
					return false;
				}

				if(rxBuffer == NULL) {
					cometos::getCout() << "ERROR: No RX buffer available" << cometos::endl;
					return false;
				}

				if(maxRxLength < fwdLength) {
					cometos::getCout() << "ERROR: Not enough space in RX buffer" << cometos::endl;
					return false;
				}

				rxBuffer[bufferPos] = value;

				if(bufferPos == fwdLength - 1) {
					// now enough data was collected

					// backup buffer and callback and set them to NULL
					// to prevent receiving new data into old buffer
					cbrx_back = cbrx;
					rxBuffer_back = rxBuffer;
					fwdLength_back = fwdLength;
					cbrx = EMPTY_CALLBACK();
					rxBuffer = NULL;

				    //if(cbrx_back) {
				    //    cbrx_back(rxBuffer_back, fwdLength_back, rxCmdNumber, rxType, COMETOS_SUCCESS);
				    //}

					cometos::getScheduler().replace(taskTx);
				}
			}
		}

		if(rxPos >= 255) {
			cometos::getCout() << "ERROR: Message too long!" << cometos::endl;
			return false;
		}

		rxPos++;

		return true;
	}
}

void PalRS485SlaveImpl::tx() {
	/** 
	 * Check data availablility 
	 */
	bool tx_data_available = (txBuffer != NULL && txLength >= 0);
	bool more_data_available = dataPending(); // Ask if more data is available


	/** 
	 * Assemble and write header
	 */
	uint8_t header[6];
	header[0] = RS485MASTERADDR;								// Reply address
	header[1] = address;									// Module address
	header[3] =  POLLFWDCMDNR;								// Command number

	if(currType == MsgType::POLL) {
		//cometos::getCout() << "Poll Received" << cometos::endl;
		if(!tx_data_available) {
			header[2] = (uint8_t)Status::NO_CMD;					// Status
			header[4] = 0;								// Length
		}
		else {
			if(more_data_available) {
				header[2] = (uint8_t)Status::MSG_AGAIN;				// Status
			}
			else {
				header[2] = (uint8_t)Status::MSG;				// Status
			}

			header[4] = txLength;							// Length
		}

	    if(txCmdNumber != 0xFF) {
	        header[3] = txCmdNumber;
	    }

		if(txStatus != 0xFF) {
		    header[2] = txStatus;
		}
	}
	else if(currType == MsgType::FWD) {
		//cometos::getCout() << "FWD Received" << cometos::endl;

		header[2] = (tx_data_available || more_data_available)
					? (uint8_t)Status::OK_AGAIN : (uint8_t)Status::OK;	// Status
		header[4] = 0;									// Length
	}
	else if(otherTypeCallback) {
	    // special type must be handled specially
	    otherTypeCallback(rxCmdNumber, rxType, &header[3], &header[2]);
        header[4] = 0;									// Length
	}
	else {
	    header[2] = 2; // invalid command
	}

	uint8_t checksum = 0;
	for(int i = 0; i <= 4; i++) {
		checksum += header[i];
	}
	header[5] = checksum;

	palRS485_write(header, sizeof(header));


	/** 
	 * Write data
	 */
	if(currType == MsgType::POLL && tx_data_available) {
		palRS485_write(txBuffer, txLength, false);
		txBuffer = NULL;
		txLength = 0;

        	this->interruptPin->set(); // high means no data available
	}


	/** 
	 * Call callbacks
	 */
	if(currType == MsgType::POLL && tx_data_available) {
		if(cbtx) {
			cbtx(COMETOS_SUCCESS);
			cbtx = EMPTY_CALLBACK();
		}
	}
// is this too late?
	else if(currType == MsgType::FWD) {
		if(cbrx_back) {
			cbrx_back(rxBuffer_back, fwdLength_back, rxCmdNumber, rxType, COMETOS_SUCCESS);
			//cbrx_back(rxBuffer_back, fwdLength_back, rxCmdNumber, COMETOS_SUCCESS);
		}
	}
}

template<> PalRS485Slave* PalRS485Slave::getInstance<int>(int port) {
	if(port == 0) {
		return &PalRS485SlaveImpl::getInstance();
	}
	else {
		return NULL;
	}
}

}

