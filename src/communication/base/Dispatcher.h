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
 * @author Stefan Unterschuetz
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "logging.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

/**Maps multiple endpoints/layers to one lower endpoint/layer.
 * Allows to map multiple upper layer to one lower layer.
 * DataRequest and DataIndication are used as exchange format.
 */
template<uint8_t N>
class Dispatcher: public Module {
public:

	Dispatcher(const char* name=NULL) :
	    Module(name),
		gateIndIn(this, &Dispatcher::handleIndication, "gateIndIn"),
				gateReqOut(this, "gateReqOut"),
				gateReqIn(this, &Dispatcher::handleRequest, "gateReqIn"),
				gateIndOut(this, "gateIndOut") {
	}

	void handleIndication(DataIndication* msg) {
		uint8_t index;

		// TODO develop standard routine to check Airframe len before deserialization
		if (msg->getAirframe().getLength() < sizeof(index)) {
		    delete(msg);
		    return;
		}
		msg->getAirframe() >> index;
		LOG_INFO("Snd Ind frm "<< msg->src << " G:"<< (uint16_t) index);
		if (gateIndOut.isConnected(index)){
            gateIndOut.send(index, msg);
		} else {
		    LOG_WARN("Output gate " << (int) index << " not connected");
		    // TODO before deleting here, we should really check if this
		    // msg was created by "new"...
		    delete(msg);
		    return;
		}
	}

	void handleRequest(DataRequest* msg) {
		uint8_t index = gateReqIn.getIndex(msg);
		msg->getAirframe() << index; // add index incoming gate
		LOG_INFO("Fwd Rq to "<< msg->dst << " G:" << (uint16_t) index);
		gateReqOut.send(msg);
	}

	InputGate<DataIndication> gateIndIn;
	OutputGate<DataRequest> gateReqOut;
	InputGateArray<DataRequest, N> gateReqIn;
	OutputGateArray<DataIndication, N> gateIndOut;

};

} /* namespace cometos */

#endif /* DISPATCHER_H_ */
