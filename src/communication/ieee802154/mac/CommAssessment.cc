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
 * @author Stefan Unterschütz
 */

#include "CommAssessment.h"

#ifdef PAL_TIME
#include "palLocalTime.h"
#endif

Define_Module(cometos::CommAssessment);

namespace cometos {

void serialize(ByteVector& buffer, const CommAssessmentResults& value) {
	serialize(buffer, value.finished);
	serialize(buffer, value.dest);
	serialize(buffer, value.payl);
	serialize(buffer, value.sent);
	serialize(buffer, value.recv);
	serialize(buffer, value.failed);
	serialize(buffer, value.timeouts);
	serialize(buffer, value.time);

}

void unserialize(ByteVector& buffer, CommAssessmentResults& value) {
	unserialize(buffer, value.time);
	unserialize(buffer, value.timeouts);
	unserialize(buffer, value.failed);
	unserialize(buffer, value.recv);
	unserialize(buffer, value.sent);
	unserialize(buffer, value.payl);
	unserialize(buffer, value.dest);
	unserialize(buffer, value.finished);
}

CommAssessment::CommAssessment(const char* name) :
		Endpoint(name), dest(BROADCAST), payl(0)
{
	resetStats();
	(void) startTime; // prevent unused warning
}

CommAssessment::~CommAssessment() {

}

void CommAssessment::resetStats() {
	finished = false;
	sent = 0;
	recv = 0;
	failed = 0;
	timeouts = 0;
	time = 0;
}

void CommAssessment::finish() {
	cancel(timeoutTimer);
	delete timeoutTimer;
}

void CommAssessment::initialize() {
	Endpoint::initialize();

	counter = 0;

	timeoutTimer = new Message;

	// make interface public
	remoteDeclare(dest, "dest");
	remoteDeclare(payl, "payl");
	remoteDeclare(&CommAssessment::rttAssement, "rtt");
	remoteDeclare(&CommAssessment::getResults, "res");
}

void CommAssessment::rttDone() {
	cancel(timeoutTimer);
	finished = true;
#ifdef PAL_TIME
	time = palLocalTime_get() - startTime;
#endif
}

void CommAssessment::handleIndication(DataIndication* msg) {

	uint8_t type;
	uint16_t c;
	msg->getAirframe() >> c >> type;
	if (type == CommAssement_RTT_RESP && c == counter) {
		counter--;
		recv++;
		if (counter > 0) {
			sendRttPacket();
			reschedule(timeoutTimer, &CommAssessment::responseTimeout,
					COMM_TIMEOUT);
		} else {
			rttDone();
		}
	} else if (type == CommAssement_RTT) {
		recv++;
		msg->getAirframe() << (uint8_t) CommAssement_RTT_RESP << c;
		sendRequest(new DataRequest(msg->src, msg->decapsulateAirframe()));
	}

	delete msg;
}

void CommAssessment::sendRttPacket() {
	Airframe *frame = new Airframe;
	sent++;
	frame->setLength(payl);
	(*frame) << (uint8_t) CommAssement_RTT << counter;
	sendRequest(new DataRequest(dest, frame));
}

void CommAssessment::handleResponse(DataResponse *msg) {
	if (msg->success == false) {
		failed++;
	}
	delete msg;
}

void CommAssessment::responseTimeout(Message* timer) {
	counter--;
	timeouts++;
	if (counter > 0) {
		sendRttPacket();
		schedule(timeoutTimer, &CommAssessment::responseTimeout, COMM_TIMEOUT);
	} else {
		rttDone();
	}
}

void CommAssessment::rttAssement(uint16_t &send) {
	if (counter != 0) {
		return;
	}

	resetStats();
#ifdef PAL_TIME
	startTime = palLocalTime_get();
#endif
	counter = send;
	sendRttPacket();
	reschedule(timeoutTimer, &CommAssessment::responseTimeout, COMM_TIMEOUT);

}

CommAssessmentResults CommAssessment::getResults() {
	CommAssessmentResults value;
	value.finished = finished;
	value.dest = dest;
	value.payl = payl;
	value.sent = sent;
	value.recv = recv;
	value.failed = failed;
	value.timeouts = timeouts;
	value.time = time;
	return value;
}

} /* namespace cometos */
