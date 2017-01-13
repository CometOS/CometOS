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


/*INCLUDES-------------------------------------------------------------------*/

#include "EndpointExample.h"
#include "NetworkTime.h"
#include "OutputStream.h"

using namespace cometos;

/*METHOD DEFINITION----------------------------------------------------------*/

Define_Module(EndpointExample);

void EndpointExample::initialize() {
	Endpoint::initialize();

	getCout() << (int) NetworkTime::get() << "-" << getName() << ":sched. initial timer" << cometos::endl;

	schedule(new Message, &EndpointExample::traffic, intrand(1000));

}

void EndpointExample::traffic(Message *msg) {

	uint16_t simpleHeader = 1234;

	DataRequest *request = new DataRequest(0, make_checked<Airframe>(),
			createCallback(&EndpointExample::handleResponse));

	// serialize header to airframe
	request->getAirframe() << simpleHeader;

	getCout() << NetworkTime::get() << "-" << getName() << ":send req " << simpleHeader << cometos::endl;

	// send data with some random offset
	sendRequest(request, intrand(1000));

	delete msg;
}

void EndpointExample::handleResponse(DataResponse* msg) {
	delete msg;
}

void EndpointExample::handleIndication(DataIndication* msg) {
	uint16_t header;
	msg->getAirframe() >> header;

	delete msg;

	schedule(new Message, &EndpointExample::traffic, intrand(5000));

}

