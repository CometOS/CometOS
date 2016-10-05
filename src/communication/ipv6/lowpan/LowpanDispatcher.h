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
 * @author Martin Ringwelski
 */

#ifndef LOWPANDISPATCHER_H_
#define LOWPANDISPATCHER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "logging.h"
#include "LowpanIndication.h"
#include "palLed.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos_v6 {

/**Maps multiple endpoints/layers to one lower endpoint/layer.
 * Allows to map multiple upper layer to one lower layer.
 * DataRequest and DataIndication are used as exchange format.
 */
template<uint8_t N>
class LowpanDispatcher: public cometos::Module {
public:

    LowpanDispatcher(const char* service_name = NULL) :
        Module(service_name),
		gateIndIn(this, &LowpanDispatcher::handleIndication, "gateIndIn"),
				gateReqOut(this, "gateReqOut"),
				gateReqIn(this, &LowpanDispatcher::handleRequest, "gateReqIn"),
				gateIndOut(this, "gateIndOut"),
				LowpanIn(this, &LowpanDispatcher::handleLowpanRequest, "LowpanIn"),
				LowpanOut(this, "LowpanOut")
	{
	}

	void handleIndication(cometos::DataIndication* msg) {
		uint8_t dispatchByte;
		// TODO develop standard routine to check Airframe len before deserialization
		if (msg->getAirframe().getLength() < sizeof(dispatchByte)) {
		    delete(msg);
		    return;
		}
		msg->getAirframe() >> dispatchByte;
		if ((dispatchByte & 0xC0) == 0) {
		    if (gateIndOut.isConnected(dispatchByte)) {
		        LOG_INFO("Snd Ind frm "<< msg->src << " G:"<< (uint16_t) dispatchByte);
                gateIndOut.send(dispatchByte, msg);
		    } else {
		        LOG_WARN("Discard frame to protocol "<< (int) dispatchByte);
		        delete(msg);
		        return;
		    }
		} else {
		    LOG_INFO("LoWPAN Frame");
		    LowpanIndication* ind = new LowpanIndication(msg, dispatchByte);
		    delete(msg);
		    LowpanOut.send(ind);
		}
	}

	void handleRequest(cometos::DataRequest* msg) {
		uint8_t index = gateReqIn.getIndex(msg);
		msg->getAirframe() << index; // add index incoming gate
		LOG_INFO("Fwd Rq to "<< msg->dst << " G:" << (uint16_t) index);
		gateReqOut.send(msg);
	}

	void handleLowpanRequest(cometos::DataRequest* msg) {
	    gateReqOut.send(msg);
	}


	cometos::InputGate<cometos::DataIndication> gateIndIn;
	cometos::OutputGate<cometos::DataRequest> gateReqOut;
	cometos::InputGateArray<cometos::DataRequest, N> gateReqIn;
	cometos::OutputGateArray<cometos::DataIndication, N> gateIndOut;

	cometos::InputGate<cometos::DataRequest> LowpanIn;
	cometos::OutputGate<LowpanIndication> LowpanOut;
};

} /* namespace cometos_v6 */

#endif /* DISPATCHER_H_ */
