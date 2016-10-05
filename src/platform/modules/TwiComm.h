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
 * @author Florian Meier
 */

#ifndef TWICOMM_H_
#define TWICOMM_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "LowerEndpoint.h"
#include "palTwiMaster.h"
#include "palTwiSlave.h"
#include "Queue.h"
#include "cometosError.h"

#include "palLed.h"
#include "IGPIOPin.h"

namespace cometos {

#define QUEUE_LENGTH		4

/*CLASS DECLARATION----------------------------------------------------------*/

/**
 * Provides I2C communication. Can be used in base station and sensor node.
 *
 * This is a singleton object
 */
class TwiComm: public cometos::LowerEndpoint {
public:
	template<typename PeripheralType>
	TwiComm(PeripheralType port, bool master, uint8_t address, IGPIOPin* interruptPin) :
       		LowerEndpoint(MODULE_NAME),
       		taskTx(*this),
       		taskSendUp(*this),
       		taskRx(*this),
		txPending(false),
		master(master),
		address(address),
		twiMaster(NULL),
		twiSlave(NULL),
                interruptPin(interruptPin)
	{
		if(master) {
			twiMaster = PalTwiMaster::getInstance<PeripheralType>(port);
                        interruptPin->setDirection(IGPIOPin::Direction::IN);
                        interruptPin->setupInterrupt(IGPIOPin::Edge::FALLING, CALLBACK_MET(&TwiComm::initRx, *this));
		}
		else {	
			twiSlave = PalTwiSlave::getInstance<PeripheralType>(port);
                        interruptPin->set();
                        interruptPin->setDirection(IGPIOPin::Direction::OUT);
		}
	}

	void initialize();

	virtual void handleRequest(cometos::DataRequest* msg);

private:
	void tx();
	void initRx();
	void initTx();
	void sendUpTask();
	void confirm(cometos::DataRequest * req, bool result, bool addTxInfo, bool isValidTxTs);

	BoundedTask<TwiComm, &TwiComm::tx> taskTx;
	BoundedTask<TwiComm, &TwiComm::sendUpTask> taskSendUp;
	BoundedTask<TwiComm, &TwiComm::initRx> taskRx;

    cometos::Airframe *rxBufferTwi = NULL;
    cometos::Airframe *rxBufferSendUp = NULL;

    /** FIXME TODO instead of using a queue of requests, use a queue of
     *  dedicated datastructures which contain meta information like seq, dst, src
     *  --- this way, the airframe size can be reduced to the mac's MAC_MAX_PAYLOAD_SIZE
     */
    Queue<cometos::DataRequest*, QUEUE_LENGTH> queue;

    bool txPending;
    bool master;

    uint8_t address;

	PalTwiMaster* twiMaster;
	PalTwiSlave* twiSlave;

    IGPIOPin* interruptPin;

	static TwiComm* instance;
	static void rxFinishedCallback(uint8_t *rxBuf, uint8_t len, cometos_error_t result);
	static void txFinishedCallback(cometos_error_t result);

    static const char * const MODULE_NAME;
};

}

#endif /* TWICOMM_H_ */
