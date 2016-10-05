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
 */

#ifndef DEMUX_H_
#define DEMUX_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "logging.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

/**Maps one upper endpoint/layer to multiple lower endpoints/layers.
 * Allows to map one upper layer to multiple lower layers.
 * DataRequest and DataIndication are used as exchange format.
 * This is usually used in combination with a Dispatcher to map between different gate assigments.
 */
template<uint8_t N>
class Demux: public Module {
public:

	Demux() :
		gateIndIn(this, &Demux::handleIndication, "gateIndIn"),
				gateReqOut(this, "gateReqOut"),
				gateReqIn(this, &Demux::handleRequest, "gateReqIn"),
				gateIndOut(this, "gateIndOut") {
	}

	void handleRequest(DataRequest* msg) {
		uint8_t index;

		// TODO develop standard routine to check Airframe len before deserialization
		if (msg->getAirframe().getLength() < sizeof(index)) {
		    delete(msg);
		    return;
		}
		msg->getAirframe() >> index;
		LOG_INFO("Demux: Snd Req to "<< msg->dst << " G:"<< (uint16_t) index);
		gateReqOut.send(index, msg);
	}

	void handleIndication(DataIndication* msg) {
		uint8_t index = gateIndIn.getIndex(msg);
		msg->getAirframe() << index; // add index incoming gate
		LOG_INFO("Demux: Fwd Ind from "<< msg->src << " G:" << (uint16_t) index);
		gateIndOut.send(msg);
	}


	InputGateArray<DataIndication,N> gateIndIn;
	OutputGateArray<DataRequest,N> gateReqOut;
	InputGate<DataRequest> gateReqIn;
	OutputGate<DataIndication> gateIndOut;

};

} /* namespace cometos */

#endif /* Demux_H_ */
