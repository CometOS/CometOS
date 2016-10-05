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

#include "CounterBasedFlooding.h"
#include "types.h"
#include "logging.h"
#include "Routing.h"

Define_Module( CounterBasedFlooding);

using namespace cometos;

bool operator==(const flooding_entry_t& a, const NwkHeader& b) {
	return (a.header.seq == b.seq && a.header.src == b.src);
}

void CounterBasedFlooding::finish() {
	Layer::finish();
	RECORD_SCALAR(forwardedStats);
}

void CounterBasedFlooding::initialize() {
	Layer::initialize();
	CONFIG_NED(maxCounter);
	CONFIG_NED(sendingOffset);
	forwardedStats = 0;
	sequence = 0;
}

void CounterBasedFlooding::handleRequest(DataRequest* msg) {
	NwkHeader nwk(msg->dst, getId(), ++sequence);
	checkAndIncrement(nwk);
	msg->getAirframe() << nwk;
	msg->dst = BROADCAST;
	forwardedStats++;
	sendRequest(msg); // first broadcast is send directly
}

void CounterBasedFlooding::handleIndication(DataIndication* msg) {
	NwkHeader nwk;
	msg->getAirframe() >> nwk;

	// discard packet if already processed
	if (checkAndIncrement(nwk)) {
		delete msg;
		return;
	}

	nwk.hops++;

	// send data to upper layer
	if (nwk.dst == getId() || nwk.dst == BROADCAST) {
		Airframe *tmp = msg->getAirframe().getCopy();
		tmp->set(new RoutingInfo(nwk.hops));
		sendIndication(new DataIndication(tmp, nwk.src, nwk.dst));
	}

	// forward data
	msg->getAirframe() << nwk;
	uint16_t offset = sendingOffset / 2 + intrand(sendingOffset);
	if (nwk.dst != getId()) {
		schedule(new DataRequest(nwk.dst, msg->decapsulateAirframe()),
				&CounterBasedFlooding::forward, offset);
	}

	delete msg;
}

void CounterBasedFlooding::forward(DataRequest *req) {

	NwkHeader header;
	req->getAirframe() >> header;
	flooding_entry_t* res;
	res = history.get(header);
	ASSERT(res != NULL);

	if (res->count < maxCounter) {
		req->getAirframe() << header;
		forwardedStats++;
		sendRequest(req);
	} else {
		delete req;
	}

}

bool CounterBasedFlooding::checkAndIncrement(NwkHeader& header) {
	flooding_entry_t* res;
	flooding_entry_t temp;

	res = history.get(header);

	if (res) {
		if (res->count < maxCounter) {
			res->count++;
		}
		return true;
	} else {
		if (history.full()) {
			history.pop();
		}
		temp.header = header;
		temp.count = 0;
		history.push(temp);

		return false;
	}
}

