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
#include "palExec.h"
#include "palRS485.h"

#include <stddef.h>

namespace cometos {

class PalRS485SlaveImpl;

class PalRS485SlaveImpl : public PalRS485Slave {
public:
	virtual cometos_error_t init(uint8_t adr, Callback<bool()> dataPending, IGPIOPin* interruptPin);
    virtual cometos_error_t restart();
	virtual cometos_error_t depositReceptionBuffer(uint8_t* rxBuf, uint8_t maxLen, palRS485Slave_rxPtr callback);
	virtual cometos_error_t depositTransmissionData(const uint8_t* txBuf, uint8_t len, palRS485Slave_txPtr callback, uint8_t cmdNumber, uint8_t status);

	static PalRS485SlaveImpl rs485;

	static PalRS485SlaveImpl& getInstance() {
		// Instantiate class
		return rs485;
	}

private:
	PalRS485SlaveImpl();
};

PalRS485SlaveImpl PalRS485SlaveImpl::rs485;

PalRS485SlaveImpl::PalRS485SlaveImpl()
{}

cometos_error_t PalRS485SlaveImpl::init(uint8_t adr, Callback<bool()> dataPending, IGPIOPin* interruptPin) {
	return COMETOS_ERROR_INVALID;
}

cometos_error_t PalRS485SlaveImpl::restart() {
    return COMETOS_ERROR_INVALID;
}

cometos_error_t PalRS485SlaveImpl::depositReceptionBuffer(uint8_t * rxBuf, uint8_t maxLen,
		palRS485Slave_rxPtr callback) {
	return COMETOS_ERROR_INVALID;
}

cometos_error_t PalRS485SlaveImpl::depositTransmissionData(const uint8_t* txBuf, uint8_t len, palRS485Slave_txPtr callback, uint8_t cmdNumber, uint8_t status) {
	return COMETOS_ERROR_INVALID;
}

template<> PalRS485Slave* PalRS485Slave::getInstance<int>(int port) {
	return NULL;
}

}

