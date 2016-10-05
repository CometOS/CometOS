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
 * CometOS Platform Abstraction Layer for TWI Master.
 *
 * @author Florian Meier
 */

#include "palTwiMaster.h"
#include "cometos.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <map>

#include <stddef.h>

namespace cometos {

class PalTwiMasterImpl : public PalTwiMaster {
    friend PalTwiMaster* getInstance<const char*>(const char *);

private:
	bool initialized = false;

	uint8_t maxRxLength;
	palTwiMaster_rxPtr cbrx = NULL;
	uint8_t * rxBuffer = NULL;
	uint8_t rxAddr = 0;
    bool rxReceiveLength = false;

	uint8_t txLength;
	palTwiMaster_txPtr cbtx = NULL;
	uint8_t * txBuffer = NULL;
	uint8_t txAddr = 0;

	int i2c_file;

	void rx() {
		ASSERT(cbrx);

		int16_t rxLength = 0;
		cometos_error_t ret = setAddress(rxAddr);

		if(ret != COMETOS_SUCCESS) {
			rxBuffer = NULL;
			rxLength = 0;
		}
		else if(rxReceiveLength) {
            rxLength = 0;
			//cometos::getCout() << "i2c read with receive length" << cometos::endl;

            uint8_t* blockBuffer = new uint8_t[maxRxLength+1]; // one extra byte needed for address

            struct i2c_msg msg;
            msg.addr = rxAddr;
            msg.flags = I2C_M_RD; 
            // I2C_M_RECV_LEN would be favorable to prevent unnecessary long reads,
            // but this only works for block lengths between 1 and 32 bytes
            msg.len = maxRxLength+1;
            msg.buf = blockBuffer;

            struct i2c_rdwr_ioctl_data msgset;
            msgset.msgs = &msg;
            msgset.nmsgs = 1;

            msg.buf[0] = 1; // used to reserve the extra byte needed for the address if I2C_M_RECV_LEN enabled

            if (ioctl(i2c_file, I2C_RDWR, &msgset) < 0) {
                cometos::getCout() << "Failed to read from the i2c bus." << cometos::endl;
                const char *err = strerror(errno);
                cometos::getCout() << err << cometos::endl;
                rxBuffer = NULL;
                rxLength = 0;
                ret = COMETOS_ERROR_FAIL;
            }
            // check if buffer is large enough
            else if(rxLength + msg.buf[0] > maxRxLength) {
                cometos::getCout() << "Buffer too small to read from i2c bus." << cometos::endl;
                ret = COMETOS_ERROR_FAIL;
            }
            else {
                // copy the data to the actual buffer
                memcpy(rxBuffer + rxLength, blockBuffer+1, msg.buf[0]);
                rxLength += msg.buf[0];
            }

            delete[] blockBuffer;

            //cometos::getCout() << "Length received via I2C " << (int)rxLength << cometos::endl;
        }
        else {
			//cometos::getCout() << "i2c read" << cometos::endl;
			rxLength = read(i2c_file,rxBuffer,rxLength);

			if(rxLength <= 0) {
				cometos::getCout() << "Failed to read from the i2c bus." << cometos::endl;
                if(rxLength == 0) {
				    cometos::getCout() << "No data received." << cometos::endl;
                }
                else {
                    const char *err = strerror(errno);
                    cometos::getCout() << err << cometos::endl;
                    rxBuffer = NULL;
                    rxLength = 0;
                }
				ret = COMETOS_ERROR_FAIL;
			}
		}

		// backup buffer and callback and set them to NULL
		// to allow for next receive command
		palTwiMaster_rxPtr cbTmp = cbrx;
		uint8_t * rxBufferTmp = rxBuffer;
		cbrx = NULL;
		rxBuffer = NULL;

		cbTmp(rxBufferTmp, rxLength, ret);
	}

	void tx() {
		ASSERT(cbtx);

		cometos_error_t ret = setAddress(txAddr);

		if(ret == COMETOS_SUCCESS) {
			//cometos::getCout() << "Transmit I2C with length " << (int)txLength << cometos::endl;
			int16_t len = write(i2c_file,txBuffer,txLength);

			if(len != txLength) {
				cometos::getCout() << "Failed to write to the i2c bus." << cometos::endl;
    				const char *err = strerror(errno);
				cometos::getCout() << err << cometos::endl;
				ret = COMETOS_ERROR_FAIL;
			}
		}

		// backup callback and set it to NULL
		// to allow for next transmit command
		palTwiMaster_txPtr cbTmp = cbtx;
		cbtx = NULL;
		txBuffer = NULL;

		cbTmp(ret);
	}

	cometos_error_t setAddress(uint8_t addr) {
		if (ioctl(i2c_file,I2C_SLAVE,addr) < 0) {
			cometos::getCout() << "Failed to acquire bus access and/or talk to slave." << cometos::endl;
			return COMETOS_ERROR_FAIL;
		}
		return COMETOS_SUCCESS;
	}

	std::string port;
	BoundedTask<PalTwiMasterImpl, &PalTwiMasterImpl::rx> rxTask; 
	BoundedTask<PalTwiMasterImpl, &PalTwiMasterImpl::tx> txTask;
	
	PalTwiMasterImpl(std::string port) :
	port(port), rxTask(*this), txTask(*this)
	{}

public:
	cometos_error_t init() {
		if (initialized) {
			return COMETOS_SUCCESS;
		}
		initialized = true;

		if ((i2c_file = open(port.c_str(),O_RDWR)) < 0) {
			cometos::getCout() << "Failed to open the bus." << cometos::endl;
			ASSERT(false);
			exit(1);
		}

		return COMETOS_SUCCESS;
	}

	cometos_error_t receive(uint8_t addr, uint8_t * rxBuf, uint8_t length, bool receiveLength,
			palTwiMaster_rxPtr callback) {
		bool busy = (rxBuffer != NULL || cbrx != NULL);
		if (busy) {
			return COMETOS_ERROR_BUSY;
		}

		if (rxBuf == NULL || callback == NULL) {
			return COMETOS_ERROR_INVALID;
		}

		cbrx = callback;
		rxBuffer = rxBuf;
		rxAddr = addr;
        maxRxLength = length;
        rxReceiveLength = receiveLength;


		cometos::getScheduler().replace(rxTask);

		return COMETOS_SUCCESS;
	}

	cometos_error_t transmit(uint8_t addr, uint8_t * txBuf,
			uint8_t len, palTwiMaster_txPtr callback) {
		bool busy = (txBuffer != NULL || cbtx != NULL);
		if (busy) {
			return COMETOS_ERROR_BUSY;
		}

		if (txBuf == NULL || callback == NULL) {
			return COMETOS_ERROR_INVALID;
		}

		cbtx = callback;
		txBuffer = txBuf;
		txLength = len;
		txAddr = addr;

		cometos::getScheduler().replace(txTask);

		return COMETOS_SUCCESS;
	}
};

template<> PalTwiMaster* PalTwiMaster::getInstance<const char*>(const char* peripheral) {
    static std::map<std::string, PalTwiMasterImpl*> handles;
    std::string s(peripheral);
    if (handles.find(s) == handles.end()) {
        handles[s] = new PalTwiMasterImpl(s);
    }
    return handles[s];
}

}
