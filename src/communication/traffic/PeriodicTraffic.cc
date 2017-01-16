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

#include "PeriodicTraffic.h"
#include "Airframe.h"
#include "logging.h"

Define_Module(cometos::PeriodicTraffic);

namespace cometos {

void PeriodicTraffic::finish() {
	recordScalar("sendCounter", sendCounter);
	recordScalar("receiveCounter", receiveCounter);

}

void PeriodicTraffic::initialize() {
	Endpoint::initialize();

	sendCounter = 0;
	receiveCounter = 0;
	dst = BROADCAST;
	interval = 2000;
	payloadSize = 50;

	CONFIG_NED(dst);
	CONFIG_NED(interval);
	CONFIG_NED(payloadSize);

	remoteDeclare(&PeriodicTraffic::get, "get");

	if (interval > 0) {
#ifdef OMNETPP
		start((static_cast<timeOffset_t>(par("start"))));
#endif
}			}

void PeriodicTraffic::start(timeOffset_t offset) {
	schedule(new Message, &PeriodicTraffic::traffic, offset);
}

void PeriodicTraffic::traffic(Message *timer) {
	// create dummy data for transmission
	AirframePtr frame = make_checked<Airframe>();
	for (uint8_t i = 0; i < payloadSize; i++) {
		(*frame) << (uint8_t) i;
	}

	LOG_INFO("send "<<frame->getLength()<<" bytes to "<<dst);
	sendRequest(new DataRequest(dst, frame, createCallback(&PeriodicTraffic::resp)));

	sendCounter++;

	schedule(timer, &PeriodicTraffic::traffic, interval);
}

void PeriodicTraffic::resp(DataResponse *response) {
	LOG_INFO("rcv rsp "<<(int)response->status);
	delete response;
}

void PeriodicTraffic::handleIndication(DataIndication* msg) {
	LOG_INFO("recv "<<(int)msg->getAirframe().getLength()<<" bytes from "<<msg->src);
	delete msg;
	receiveCounter++;
}
StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> PeriodicTraffic::get(uint8_t& length,
		uint8_t& start) {
    StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> ret;

	for (uint8_t i = 0; i < length; i++) {
		ret.push_back(start++);
	}

	return ret;

}

void serialize(ByteVector & buf,
		const StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list) {
	for (uint8_t it = list.begin(); it != list.end(); it = list.next(it)) {
		serialize(buf, list.get(it));
	}
	serialize(buf, list.size());

	//cout << "Serialized List, size " << (int) buf.getSize() << endl;
}

void unserialize(ByteVector & buf, StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list) {
	//cout << "Unserialize List, size " << (int) buf.getSize() << endl;

	uint8_t size;
	list.clear();
	unserialize(buf, size);

	uint8_t item;
	for (uint8_t i = 0; i < size; i++) {
		unserialize(buf, item);
		list.push_front(item);
	}
}
uint8_t getTrafficPayload(StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list
		, uint8_t index) {
	uint8_t it = list.begin();
	for (; it != list.end() && index != 0; it = list.next(it)) {
		index--;
	}
	if (it != list.end())
		return list[it];;
	return 0;
}

} /* namespace cometos */
