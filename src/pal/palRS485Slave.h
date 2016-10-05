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

#ifndef PALRS485SLAVE_H_
#define PALRS485SLAVE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "cometos.h"
#include "cometosError.h"
#include "IGPIOPin.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class PalRS485Slave {
public:
	typedef Callback<void(uint8_t *rxBuf, uint8_t len, uint8_t cmdNumber, uint8_t type, cometos_error_t result)> palRS485Slave_rxPtr;
	typedef Callback<void(cometos_error_t result)> palRS485Slave_txPtr;

	/**
	 * Initializes RS485 module.
	 * @param adr RS485 address of the slave.
	 * @param dataPending Callback that returns true if more data is pending.
	 * @param interruptPin Pin to signalize deposited transmission data (high = no data available)
	 */
	virtual cometos_error_t init(uint8_t adr, Callback<bool()> dataPending, IGPIOPin* interruptPin) = 0;

	/**
	 * Setup the lower RS485 layers after another module has used them.
	 */
	virtual cometos_error_t restart();

	/**
	 * Deposit buffer that will be written to when the RS485 master issues a write command.
	 *
	 * @param rxBuf Buffer that will contain the received data.
	 *				The buffer has to stay valid until the callback is called.
	 * @param maxLen Number of bytes in the buffer.
	 * @param callback Callback is called when data was received.
	 */
	virtual cometos_error_t depositReceptionBuffer(uint8_t * rxBuf, uint8_t maxLen, palRS485Slave_rxPtr callback) = 0;

	/**
	 * Deposit data that will be sent when the RS485 master issues a read command.
	 *
	 * This does not actively start a transmission, since this is not possible
	 * for an RS485 slave. When you need such a functionality, you have to signal
	 * this to the master by an external interrupt line. 
	 *
	 * @param txBuf Buffer with data for transmission.
	 *				The buffer has to stay valid until the callback is called.
	 * @param len	Number of bytes to transmit
	 * @param callback Callback is called when data was finally transmitted.
	 */
	virtual cometos_error_t depositTransmissionData(const uint8_t * txBuf, uint8_t len, palRS485Slave_txPtr callback, uint8_t cmdNumber = 0xFF, uint8_t status = 0xFF) = 0;

	/**Returns an instance of PalRS485Slave.
	 * @param port if platform supports multiple ports, these can be selected with this parameter
 	 * @return PalSerial instance matching the given port
 	 */
	template<typename PortType>
	static PalRS485Slave* getInstance(PortType port);

    void setOtherTypeCallback(Callback<void(uint8_t rxcmd, uint8_t rxtype, uint8_t* txcmd, uint8_t* txstatus)> otherTypeCallback) {
        this->otherTypeCallback = otherTypeCallback;
    }

protected:
	PalRS485Slave() {}
    Callback<void(uint8_t,uint8_t,uint8_t*,uint8_t*)> otherTypeCallback;
};

}

#endif /* PALRS485SLAVE_H_ */

