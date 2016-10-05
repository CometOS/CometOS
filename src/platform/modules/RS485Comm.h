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

#ifndef RS485COMM_H_
#define RS485COMM_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "LowerEndpoint.h"
#include "palRS485Master.h"
#include "palRS485Slave.h"
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
class RS485Comm : public cometos::LowerEndpoint {
public:
	template<typename PeripheralType>
	RS485Comm(PeripheralType port, bool master, uint8_t masterAddress, uint8_t slaveAddress, IGPIOPin* interruptPin) :
       		LowerEndpoint(MODULE_NAME),
       		taskTx(*this),
       		taskSendUp(*this),
       		taskRx(*this),
       		rxBufferRS485(NULL),
        rxBufferSendUp(NULL),
		txPending(false),
		master(master),
		slaveAddress(slaveAddress),
		masterAddress(masterAddress),
		rs485Master(NULL),
		rs485Slave(NULL),
                interruptPin(interruptPin)
	{
		if(master) {
			rs485Master = PalRS485Master::getInstance<PeripheralType>(port);
                        interruptPin->setDirection(IGPIOPin::Direction::IN);
                        interruptPin->pullup(true);
                        interruptPin->setupInterrupt(IGPIOPin::Edge::FALLING, CALLBACK_MET(&RS485Comm::initRx, *this));
		}
		else {	
			rs485Slave = PalRS485Slave::getInstance<PeripheralType>(port);
		}
	}

	void initialize();
	void restart();

	virtual void handleRequest(cometos::DataRequest* msg);

private:
	void tx();
	void initRx();
	void initTx();
	void sendUpTask();
	void confirm(cometos::DataRequest * req, bool result, bool addTxInfo, bool isValidTxTs);
	bool dataPending();

	BoundedTask<RS485Comm, &RS485Comm::tx> taskTx;
	BoundedTask<RS485Comm, &RS485Comm::sendUpTask> taskSendUp;
	BoundedTask<RS485Comm, &RS485Comm::initRx> taskRx;

    cometos::Airframe *rxBufferRS485;
    cometos::Airframe *rxBufferSendUp;

    /** FIXME TODO instead of using a queue of requests, use a queue of
     *  dedicated datastructures which contain meta information like seq, dst, src
     *  --- this way, the airframe size can be reduced to the mac's MAC_MAX_PAYLOAD_SIZE
     */
    Queue<cometos::DataRequest*, QUEUE_LENGTH> queue;

    bool txPending;
    bool master;

    uint8_t slaveAddress;
    uint8_t masterAddress;

	PalRS485Master* rs485Master;
	PalRS485Slave* rs485Slave;

    IGPIOPin* interruptPin;

	static RS485Comm* instance;
	void rxFinishedCallback(uint8_t *rxBuf, uint8_t len, uint8_t cmdNumber, uint8_t typeOrStatus, cometos_error_t result);
	void txFinishedCallback(cometos_error_t result);

    static const char * const MODULE_NAME;
};

}

#endif /* RS485COMM_H_ */
