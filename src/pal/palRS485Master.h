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

#ifndef PALRS485MASTER_H_
#define PALRS485MASTER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "cometos.h"
#include "cometosError.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class PalRS485Master {
public:
	typedef Callback<void(uint8_t *rxBuf, uint8_t len, uint8_t cmdNumber, uint8_t status, cometos_error_t result)> palRS485Master_rxPtr;
	typedef Callback<void(cometos_error_t result)> palRS485Master_txPtr;

	/**
	 * Initializes RS485 module.
	 * @param master_adr Address of the master.
	 */
	virtual cometos_error_t init(uint8_t master_adr) = 0;

	/**
	 * Receive data from RS485 slave.
	 *
	 * @param slave_adr Address of the slave.
	 * @param rxBuf Buffer that will contain the received data.
	 *				The buffer has to stay valid until the callback is called.
	 * @param length Number of bytes to receive or maximum number of bytes to receive if receiveLength is true
	 * @param callback Callback is called when data was received.
	 */
	virtual cometos_error_t receive(uint8_t slave_adr, uint8_t * rxBuf, uint8_t maxLen, uint8_t cmdNumber, palRS485Master_rxPtr callback) = 0;

	/**
	 * Transmit data to RS485 slave.
	 *
	 * @param slave_adr Address of the slave.
	 * @param txBuf Buffer with data for transmission.
	 *				The buffer has to stay valid until the callback is called.
	 * @param len	Number of bytes to transmit
	 * @param callback Callback is called when data was finally transmitted.
	 */
	virtual cometos_error_t transmit(uint8_t slave_adr, const uint8_t * txBuf, uint8_t len, palRS485Master_txPtr callback, uint8_t cmdNumber, uint8_t type) = 0;

	/**Returns an instance of PalRS485Master.
	 * @param port if platform supports multiple ports, these can be selected with this parameter
 	 * @return PalSerial instance matching the given port
 	 */
	template<typename PortType>
	static PalRS485Master* getInstance(PortType port);

protected:
	PalRS485Master() {}
};

}

#endif /* PALRS485MASTER_H_ */

