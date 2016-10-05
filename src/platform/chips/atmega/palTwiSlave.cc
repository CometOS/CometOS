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
 * CometOS Platform Abstraction Layer for TWI Slave.
 *
 * @author Florian Meier
 */

#include "palTwiSlave.h"
#include <avr/interrupt.h>
#include "palExec.h"
#include <util/twi.h>

#include <stddef.h>

class PalTwiSlaveImpl : public PalTwiSlave {
private:
	bool initialized;

	volatile uint8_t rxPos;
	uint8_t maxRxLength;
	palTwiSlave_rxPtr cbrx;
	uint8_t * rxBuffer;

	volatile uint16_t txPos;
	uint8_t txLength;
	palTwiSlave_txPtr cbtx;
	uint8_t * txBuffer;
    bool sendLen;
	
	PalTwiSlaveImpl()
	: initialized(false), rxPos(0), maxRxLength(0), cbrx(NULL), rxBuffer(NULL),
	txPos(0), txLength(0), cbtx(NULL), txBuffer(NULL), sendLen(false)
	{}

	inline void palTwiSlave_sendACK() {
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA) | (0 << TWSTA)
				| (0 << TWSTO) | (0 << TWWC);
	}

	inline void palTwiSlave_expectACK() {
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA) | (0 << TWSTA)
				| (0 << TWSTO) | (0 << TWWC);
	}

	inline void palTwiSlave_sendNACK() {
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (0 << TWEA) | (0 << TWSTA)
				| (0 << TWSTO) | (0 << TWWC);
	}

	inline void palTwiSlave_lastByteSent() {
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (0 << TWEA) | (0 << TWSTA)
				| (0 << TWSTO) | (0 << TWWC);
	}

	inline void palTwiSlave_reset() {
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA) | (0 << TWSTA)
				| (0 << TWSTO) | (0 << TWWC);
	
	}

public:
	virtual cometos_error_t init(uint8_t adr) {
		if (initialized) {
			return COMETOS_SUCCESS;
		}
		initialized = true;

		TWAR = adr << 1;

		palTwiSlave_reset();

		return COMETOS_SUCCESS;
	}

	virtual cometos_error_t depositReceptionBuffer(uint8_t * rxBuf, uint8_t maxLen,
			palTwiSlave_rxPtr callback) {
		palExec_atomicBegin();
		bool busy = (rxBuffer != NULL || cbrx != NULL);
		if (busy) {
			palExec_atomicEnd();
			return COMETOS_ERROR_BUSY;
		}

		if (rxBuf == NULL || callback == NULL) {
			palExec_atomicEnd();
			return COMETOS_ERROR_INVALID;
		}

		cbrx = callback;
		rxBuffer = rxBuf;
		rxPos = 0;
		maxRxLength = maxLen;

		palExec_atomicEnd();

		return COMETOS_SUCCESS;
	}

	virtual cometos_error_t depositTransmissionData(uint8_t * txBuf,
			uint8_t len, bool sendLength, palTwiSlave_txPtr callback) {
		palExec_atomicBegin();
		bool busy = (txBuffer != NULL || cbtx != NULL);
		if (busy) {
			palExec_atomicEnd();
			return COMETOS_ERROR_BUSY;
		}

		if (txBuf == NULL || callback == NULL) {
			palExec_atomicEnd();
			return COMETOS_ERROR_INVALID;
		}

		cbtx = callback;
		txBuffer = txBuf;
		txPos = 0;
		txLength = len;
        sendLen = sendLength;

		palExec_atomicEnd();

		return COMETOS_SUCCESS;
	}

	void twi_vect() {
		switch (TW_STATUS) {
		// Slave Receiver
		case TW_SR_SLA_ACK: // 0x60, Slave was addressed as receiver
			palTwiSlave_sendACK();
			txPos = 0;
			rxPos = 0;
			break;

		case TW_SR_DATA_ACK: // 0x80, Data was received
			if (rxPos < maxRxLength && rxBuffer != NULL) {
				rxBuffer[rxPos] = TWDR;
				rxPos++;
				palTwiSlave_sendACK();
			}
			else {
				palTwiSlave_sendNACK();
			}
			break;

		case TW_SR_STOP: // 0xA0, Stop signal was received
			if (rxBuffer != NULL && cbrx != NULL) {
				// backup buffer and callback and set them to NULL
				// to prevent receiving new data into old buffer
				palTwiSlave_rxPtr cbTmp = cbrx;
				uint8_t * rxBufferTmp = rxBuffer;
				uint16_t posTmp = rxPos;
				cbrx = NULL;
				rxBuffer = NULL;

				palTwiSlave_sendACK();

				// execute callback
				cbTmp(rxBufferTmp, posTmp, COMETOS_SUCCESS);
			} else {
				palTwiSlave_sendNACK();
			}

			break;

		//Slave transmitter
		case TW_ST_SLA_ACK: //0xA8, Slave was addressed as transmitter
            // send length if requested
            if (sendLen || txBuffer == NULL) {
                if(txPos >= txLength) {
                    TWDR = 0;
			  	    palTwiSlave_lastByteSent();
                }
                /* only needed for smbus compatiblity
                else if(txLength-txPos >= I2C_SMBUS_BLOCK_MAX) {
                    TWDR = I2C_SMBUS_BLOCK_MAX;
                    palTwiSlave_expectACK();
                }
                */
                else {
                    TWDR = txLength-txPos;
                    palTwiSlave_expectACK();
                }
                break;
            }
            // else if no length was sent, continue with next case and send first byte
		case TW_ST_DATA_ACK: //0xB8, Byte was sent
            if (txPos < txLength || txBuffer != NULL) {
			  	TWDR = txBuffer[txPos];
			   	txPos++;
			}

			if (txPos >= txLength) {
			  	palTwiSlave_lastByteSent();
			} else {
			    palTwiSlave_expectACK();
			}
			break;
		case TW_ST_DATA_NACK: // 0xC0, Send no more data
		case TW_ST_LAST_DATA: // 0xC8, Last byte was transmitted
            if (txPos >= txLength) {
                // backup callback and set it and the buffer to NULL
                // to prevent sending data again
                palTwiSlave_txPtr cbTmp = cbtx;
                cbtx = NULL;
                txBuffer = NULL;
                txLength = 0;
                txPos = 0;

                palTwiSlave_reset();

                // execute callback
                if(cbTmp) {
                    cbTmp(COMETOS_SUCCESS);
                }
            }
            else {
                // block was finished, but there is still data left
                palTwiSlave_reset();
            }
			break;
		case TW_SR_DATA_NACK: // 0x88, NACK returned
		default:
			palTwiSlave_reset();
			break;
		}
	}
	
public:
	static PalTwiSlaveImpl& getInstance() {
		// Instantiate class
		static PalTwiSlaveImpl twi;
		return twi;
	}
};

template<> PalTwiSlave* PalTwiSlave::getInstance<int>(int port) {
	if(port == 0) {
		return &PalTwiSlaveImpl::getInstance();
	}
	else {
		return NULL;
	}
}

ISR(TWI_vect) {
	PalTwiSlaveImpl::getInstance().twi_vect();
}

