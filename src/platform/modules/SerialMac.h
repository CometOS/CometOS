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

#ifndef SIMPLE_CSMA_H_
#define SIMPLE_CSMA_H_

#include "Layer.h"
#include "DataRequest.h"
#include "Queue.h"
#include "Callback.h"

namespace cometos {

/**
 * A module that emulates a MAC layer by utilizing a serial-to-wireless-bridge
 * (e.g., TimeSyncWirelessBridge). This layer relies on SerialComm, which
 * serializes meta information (like MacControl to the wireless interface or
 * MacRxInfo from the wireless interface of the serial-to-wireless-bridge) and
 * passes those as objects attached to the DataIndication/DataRequest objects.
 *
 * SerialMac will also only issue a DataResponse when/if finally a feedback from
 * the serial interface arrives, that the wireless bridge actually sent the
 * wireless packet (and not when the SerialComm has finished sending). Thereby,
 * it can also provide MacRxInfo and MacTxInfo from the actually attached
 * serial interface.
 */
class SerialMac: public Layer {
public:
    static const uint8_t QUEUE_SIZE = 10;
    static const timeOffset_t SERIAL_RESPONSE_TIMEOUT = 250;

	SerialMac(const char * name = NULL);

	virtual void initialize();

	virtual void finish();

	virtual void handleIndication(DataIndication* msg);

	virtual void handleResponse(DataResponse* resp);

	virtual void handleRequest(DataRequest* msg);

	void responseTimeout();

	void setPromiscuousMode(bool promiscuousMode);

	bool getPromiscuousMode();

	OutputGate<DataIndication> snoopIndOut;

private:
	void sendNext();

	TypedDelegate<DataResponse>* originalResponseTarget;
	Queue<DataRequest*, QUEUE_SIZE> reqQueue;
	CallbackTask responseTimeoutTask;
	uint8_t seq;
    RequestId* currReqId;
	bool promiscuousMode;
};

}

#endif /* SIMPLE_MAC_H_ */
