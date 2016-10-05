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
 * CometOS Platform Abstraction Layer for RS485 Master.
 *
 * @author Florian Meier
 */

#include "palRS485Master.h"
#include "palExec.h"
#include "palRS485.h"

#include <stddef.h>

#define TIMEOUT 2000

namespace cometos {

class PalRS485MasterImpl;

class PalRS485MasterImpl : public PalRS485Master {
private:
	bool initialized = false;

	uint8_t replyPos;
	uint8_t maxRxLength;
	palRS485Master_rxPtr cbrx;
	uint8_t * rxBuffer;
	uint8_t rxCmdNumber;

	palRS485Master_rxPtr cbrx_back;
	uint8_t * rxBuffer_back;
	uint8_t pollLength_back;

	uint8_t txLength;
	palRS485Master_txPtr cbtx;
	const uint8_t* txBuffer;
	uint8_t txCmdNumber;

	uint8_t master_address;
	uint8_t tx_slave_address;
	uint8_t rx_slave_address;
	uint8_t pollLength;
	uint8_t replyChecksum;
	uint8_t replyCmdNumber;
	uint8_t rxStatus;

	enum class MsgType : uint8_t {
		POLL = 0,
		FWD = 1,
		INVALID = 0xFF
	};

	MsgType currType;

	enum class Status : uint8_t {
		OK = 100,
		OK_AGAIN = 102,
		NO_CMD = 64,
		MSG = 103,
		MSG_AGAIN = 104
	};

public:
	virtual cometos_error_t init(uint8_t master_adr);
	virtual cometos_error_t receive(uint8_t slave_adr, uint8_t * rxBuf, uint8_t maxLen, uint8_t cmdNumber, palRS485Master_rxPtr callback);
	virtual cometos_error_t transmit(uint8_t slave_adr, const uint8_t * txBuf, uint8_t len, palRS485Master_txPtr callback, uint8_t cmdNumber, uint8_t type = (uint8_t)MsgType::FWD);

	static PalRS485MasterImpl rs485;

	static PalRS485MasterImpl& getInstance() {
		// Instantiate class
		return rs485;
	}

private:
	PalRS485MasterImpl();

	bool rxByteCallback(uint8_t value, bool flag);
	void initiateCommunication();
	void tx();
	void rx();
	void abort();

	bool txActive;
	bool rxActive;

	BoundedTask<PalRS485MasterImpl, &PalRS485MasterImpl::initiateCommunication> taskComm;
	BoundedTask<PalRS485MasterImpl, &PalRS485MasterImpl::abort> taskTimeout;
};

PalRS485MasterImpl PalRS485MasterImpl::rs485;

PalRS485MasterImpl::PalRS485MasterImpl()
: initialized(false), replyPos(0), maxRxLength(0), cbrx(), rxBuffer(NULL), cbrx_back(), rxBuffer_back(NULL), pollLength_back(0), 
txLength(0), cbtx(), txBuffer(NULL), txActive(false), rxActive(false), taskComm(*this), taskTimeout(*this)

{}

cometos_error_t PalRS485MasterImpl::init(uint8_t master_adr) {
	if (initialized) {
		return COMETOS_SUCCESS;
	}

	master_address = master_adr;

	palRS485_init(RS485_BAUDRATE,NULL,NULL,NULL);
	palRS485_9bit_mode(true,true,CALLBACK_MET(&PalRS485MasterImpl::rxByteCallback, *this));

	initialized = true;

	return COMETOS_SUCCESS;
}

cometos_error_t PalRS485MasterImpl::receive(uint8_t slave_adr, uint8_t * rxBuf, uint8_t maxLen, uint8_t cmdNumber, palRS485Master_rxPtr callback) {
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

	rx_slave_address = slave_adr;
	cbrx = callback;
	rxBuffer = rxBuf;
	maxRxLength = maxLen;
	rxCmdNumber = cmdNumber;

	palExec_atomicEnd();

	cometos::getScheduler().replace(taskComm);

	return COMETOS_SUCCESS;
}

cometos_error_t PalRS485MasterImpl::transmit(uint8_t slave_adr, const uint8_t * txBuf, uint8_t len, palRS485Master_txPtr callback, uint8_t cmdNumber, uint8_t type) {
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

	tx_slave_address = slave_adr;
	cbtx = callback;
	txBuffer = txBuf;
	txLength = len;
	txCmdNumber = cmdNumber;

	palExec_atomicEnd();

	cometos::getScheduler().replace(taskComm);

	return COMETOS_SUCCESS;
}

void PalRS485MasterImpl::abort() {
	if(txActive) {
		cometos::getCout() << "Abort in PalRS485MasterImpl during txActive" << endl;

	    palExec_atomicBegin();
	    auto cbtx_back = cbtx;
	    txBuffer = NULL;
	    cbtx = EMPTY_CALLBACK();
        palExec_atomicBegin();

        if(cbtx_back) {
            cbtx_back(COMETOS_ERROR_FAIL);
        }
	}

	if(rxActive) {
		cometos::getCout() << "Abort in PalRS485MasterImpl during rxActive" << endl;

        palExec_atomicBegin();
        auto cbrx_back = cbrx;
        auto rxBuffer_back = rxBuffer;
        rxBuffer = NULL;
        cbrx = EMPTY_CALLBACK();
        palExec_atomicBegin();

        if(cbrx_back) {
            cbrx_back(rxBuffer_back,0,0,0,COMETOS_ERROR_FAIL);
        }
	}

	txActive = rxActive = false;


}

bool PalRS485MasterImpl::rxByteCallback(uint8_t value, bool flag) {
	if(replyPos == 0 && !flag) {
		cometos::getCout() << "ERROR: Out of sync! replyPos == 0, but flag not set!" << cometos::endl;
		return false;
	}

	if(flag) {
		/* TODO Flush previous message if garbage? */

		replyPos = 1;
		replyChecksum = value;

		if(value != master_address) {
			// not for me
			cometos::getCout() << "ERROR: Reply address " << (int32_t)value << " does not match master address " << master_address << cometos::endl;
			goto error;
		}
		else {
			// for me
			return true;
		}
	}
	else {
		if(replyPos <= 4) {
			replyChecksum += value;
		}

		if(replyPos == 1) {
			// Module address
			uint8_t adr = txActive ? tx_slave_address : rx_slave_address;
			if(value != adr) {
				cometos::getCout() << "ERROR: Module address " << (int32_t)value << " does not match slave address " << adr << cometos::endl;
			}
		}
		else if(replyPos == 2) {
			rxStatus = value;
		}
		else if(replyPos == 3) {
			// Command Number
			replyCmdNumber = value;
		}
		else if(replyPos == 4) {
			if(txActive && value != 0) {
				cometos::getCout() << "ERROR: POLL type, but length != 0" << cometos::endl;
				goto error;
			}
			else {
				pollLength = value;
			}
		}
		else if(replyPos == 5) {
			if(replyChecksum != value) {
				cometos::getCout() << "ERROR: Invalid checksum! Should be " << (int32_t)replyChecksum << " is " << (int32_t)value << cometos::endl;
				goto error;
			}

			// header finished
		
			if(txActive) {	
				txBuffer = NULL;
				txLength = 0;

				if(cbtx) {
					cbtx(COMETOS_SUCCESS);
					cbtx = EMPTY_CALLBACK();
				}

				txActive = false;

				// continue with the next communication if necessary
				cometos::getScheduler().replace(taskComm);
			}
			else {
				switch(rxStatus) {
				case 64:
					// Status No Commands
					rxActive = false;
					break;
				case 103:
					// Status OK Message Attached
					// OK -> continue in next byte
					break;
				default:
				    // another status byte was received
				    // -> no payload, but call the callback anyway
                    if(cbrx) {
                        cbrx(rxBuffer, 0, replyCmdNumber, rxStatus, COMETOS_SUCCESS);
                        cbrx = EMPTY_CALLBACK();
                    }

                    rxBuffer = NULL;

                    rxActive = false;
                    break;
				}
			}
		}
		else {
			if(txActive) {
				cometos::getCout() << "ERROR: Encapsulated command, but not FWD " << (int32_t)value << cometos::endl;
				goto error;
			}
			else {
				uint8_t bufferPos = replyPos-6;

				if(bufferPos >= pollLength) {
					cometos::getCout() << "ERROR: More bytes encapsulated as given by the length" << cometos::endl;
					goto error;
				}

				if(rxBuffer == NULL) {
					cometos::getCout() << "ERROR: No RX buffer available" << cometos::endl;
					goto error;
				}

				if(maxRxLength < pollLength) {
					cometos::getCout() << "ERROR: Not enough space in RX buffer" << cometos::endl;
					goto error;
				}

				rxBuffer[bufferPos] = value;

				if(bufferPos == pollLength - 1) {
					// now enough data was collected
					if(cbrx) {
						cbrx(rxBuffer, pollLength, replyCmdNumber, rxStatus, COMETOS_SUCCESS);
						cbrx = EMPTY_CALLBACK();
					}

					rxBuffer = NULL;

					rxActive = false;

					// continue with the next communication if necessary
					cometos::getScheduler().replace(taskComm);
				}
			}
		}

		if(replyPos >= 255) {
			cometos::getCout() << "ERROR: Message too long!" << cometos::endl;
			goto error;
		}

		replyPos++;

		return true;
	}

error:
    abort();
	return false;
}

void PalRS485MasterImpl::tx() {
	txActive = true;
	cometos::getScheduler().replace(taskTimeout,TIMEOUT);
	
	/**
	 * Transmit packet
	 */

	// Assemble and write header
	uint8_t header[5];
	header[0] = tx_slave_address;			// Reply address
	header[1] = txCmdNumber;			// Command number
	header[2] = (uint8_t)MsgType::FWD;		// Command type
	header[3] = txLength;				// Length

	uint8_t checksum = 0;
	for(int i = 0; i <= 3; i++) {
		checksum += header[i];
	}
	header[4] = checksum;				// Checksum

	palRS485_write(header, sizeof(header));


	// Write data
	palRS485_write(txBuffer, txLength, false);

	
	/**
	 * Now wait for the ACK
	 */
}

void PalRS485MasterImpl::rx() {
	rxActive = true;
	cometos::getScheduler().replace(taskTimeout,TIMEOUT);
	
	/**
	 * Transmit poll request
	 */

	// Assemble and write header
	uint8_t header[5];
	header[0] = rx_slave_address;			// Reply address
	header[1] = rxCmdNumber;			// Command number
	header[2] = (uint8_t)MsgType::POLL;		// Command type
	header[3] = 0;					// Length

	uint8_t checksum = 0;
	for(int i = 0; i <= 3; i++) {
		checksum += header[i];
	}
	header[4] = checksum;				// Checksum

	palRS485_write(header, sizeof(header));

	/**
	 * Now wait for data
	 */
}

// used to transmit data to the slave (forwarding or polling)
// only to be called from task context
void PalRS485MasterImpl::initiateCommunication() {
	if(rxActive || txActive) {
		// will be called again after the transmission is finished
		return;
	}

	if(txBuffer != NULL && rxBuffer != NULL) {
		static bool fairnessToggle = true;
		fairnessToggle = !fairnessToggle;
		if(fairnessToggle) {
			tx();
		}
		else {
			rx();
		}
	}
	else if(txBuffer != NULL) {
		tx();
	}
	else if(rxBuffer != NULL) {
		rx();
	}
}

template<> PalRS485Master* PalRS485Master::getInstance<int>(int port) {
	if(port == 2) {
		return &PalRS485MasterImpl::getInstance();
	}
	else {
		return NULL;
	}
}

}

