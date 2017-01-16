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

#include "UDPComm.h"
#include "cometos.h"
#include "DummyEndpoint.h"
#include "SimpleReliabilityLayer.h"

#define GEN_DATA_INTERVAL 6000
#define MASTER_PORT 	1234
#define SLAVE_PORT 	5432

using namespace cometos;

#ifdef MASTER
static UDPComm udp(MASTER_PORT, "127.0.0.1", SLAVE_PORT);
#else
static UDPComm udp(SLAVE_PORT); // dynamic remote
#endif

static cometos::DummyEndpoint ep;
static cometos::SimpleReliabilityLayer reliability("sr");

void generateRequest();
SimpleTask task(generateRequest);
void generateRequest() {
	cometos::getCout() << "Generate Data" << cometos::endl;
	cometos::getScheduler().add(task, GEN_DATA_INTERVAL);

	cometos::AirframePtr frame = cometos::make_checked<cometos::Airframe>();
	frame->setLength(2);
	frame->getData()[0] = 4;
	frame->getData()[1] = 5;

	cometos::DataRequest* req = new cometos::DataRequest(0x00,frame);
	//req->getAirframe().print();

	ep.send(req);
}

void receiveIndication(cometos::DataIndication* msg) {
	cometos::getCout() << "receiveIndication: " << cometos::endl;
	msg->getAirframe().printFrame();

	delete msg;
}

int main(int argc, const char *argv[]) {
	reliability.gateReqOut.connectTo(udp.gateReqIn);
	udp.gateIndOut.connectTo(reliability.gateIndIn);

	reliability.gateIndOut.connectTo(ep.gateIndIn);
	ep.gateReqOut.connectTo(reliability.gateReqIn);

	cometos::getScheduler().add(task);
    	ep.setCallback(CALLBACK_FUN(receiveIndication));

	cometos::initialize();
	cometos::run();
	return 0;
}

