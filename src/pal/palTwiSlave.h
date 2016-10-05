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

#ifndef PALTWISLAVE_H_
#define PALTWISLAVE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "cometosError.h"

/*PROTOTYPES-----------------------------------------------------------------*/

class PalTwiSlave {
public:
	typedef void (*palTwiSlave_rxPtr) (uint8_t *rxBuf, uint8_t len, cometos_error_t result);
	typedef void (*palTwiSlave_txPtr) (cometos_error_t result);

	/**
	 * Initializes TWI module.
	 * @param adr TWI address of the slave.
	 */
	virtual cometos_error_t init(uint8_t adr) = 0;

	/**
	 * Deposit buffer that will be written to when the TWI master issues a write command.
	 *
	 * @param rxBuf Buffer that will contain the received data.
	 *				The buffer has to stay valid until the callback is called.
	 * @param maxLen Number of bytes in the buffer.
	 * @param callback Callback is called when data was received.
	 */
	virtual cometos_error_t depositReceptionBuffer(uint8_t * rxBuf, uint8_t maxLen, palTwiSlave_rxPtr callback) = 0;

	/**
	 * Deposit data that will be sent when the TWI master issues a read command.
	 *
	 * This does not actively start a transmission, since this is not possible
	 * for an TWI slave. When you need such a functionality, you have to signal
	 * this to the master by an external interrupt line. 
	 *
	 * @param txBuf Buffer with data for transmission.
	 *				The buffer has to stay valid until the callback is called.
	 * @param len	Number of bytes to transmit
     * @param sendLength    Send the length of the remaining data as first byte
	 * @param callback Callback is called when data was finally transmitted.
	 */
	virtual cometos_error_t depositTransmissionData(uint8_t * txBuf, uint8_t len, bool sendLength, palTwiSlave_txPtr callback) = 0;

	/**Returns an instance of PalTwiSlave.
	 * @param port if platform supports multiple ports, these can be selected with this parameter
 	 * @return PalSerial instance matching the given port
 	 */
	template<typename PortType>
	static PalTwiSlave* getInstance(PortType port);

protected:
	PalTwiSlave() {}
};

#endif /* PALTWISLAVE_H_ */
