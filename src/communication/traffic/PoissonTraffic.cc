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

#include "PoissonTraffic.h"
#include "Airframe.h"
#include "logging.h"

Define_Module(cometos::PoissonTraffic);

namespace cometos {

void PoissonTraffic::finish() {
	recordScalar("sendCounter", sendCounter);
	recordScalar("receiveCounter", receiveCounter);

}

void PoissonTraffic::initialize() {
	Endpoint::initialize();
	dst = par("dst");
	interval = par("interval");
	payloadSize = par("payloadSize");

	sendCounter = 0;
	receiveCounter = 0;

	if (interval > 0) {
		schedule(new Message, &PoissonTraffic::traffic,
				getOffset() + static_cast<double>(par("start")));

	}
}

double PoissonTraffic::getOffset() {
	return (-log(dblrand()) * interval);
}

void PoissonTraffic::traffic(Message *timer) {
	// create dummy data for transmission
	AirframePtr frame = make_checked<Airframe>();
	for (uint8_t i = 0; i < payloadSize; i++) {
		(*frame) << i;
	}

	LOG_INFO("send dt "<<frame->getLength());
	sendCounter++;
	sendRequest(new DataRequest(dst, frame, createCallback(&PoissonTraffic::resp)));

	schedule(timer, &PoissonTraffic::traffic, getOffset());
}

void PoissonTraffic::resp(DataResponse *response) {
	LOG_INFO("rcv rsp "<<response->success);
	delete response;
}

void PoissonTraffic::handleIndication(DataIndication* msg) {
	LOG_INFO("rcv dt "<<(int)msg->getAirframe().getLength());
	delete msg;
	receiveCounter++;
}

} /* namespace cometos */
