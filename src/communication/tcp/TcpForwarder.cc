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

#include "TcpForwarder.h"
#include "mac_definitions.h"

using namespace cometos;

TcpForwarder::TcpForwarder() :
		pTimer(NULL) {
}

void TcpForwarder::initialize() {
	Endpoint::initialize();

	pTimer = new cometos::Message;
	schedule(pTimer, &TcpForwarder::run, TCP_COMM_POLL);

}

void TcpForwarder::run(cometos::Message* msg) {
	schedule(pTimer, &TcpForwarder::run, TCP_COMM_POLL);
	pollAll();
}


void TcpForwarder::handleIndication(cometos::DataIndication* msg) {
	for (std::set<socket_t>::iterator it = conn.begin(); it != conn.end(); it++) {
		TcpAgent::send(*it, msg->getAirframe().getData(),
				msg->getAirframe().getLength());
	}
	delete msg;

	return;
}

void TcpForwarder::receivedCallback(const socket_t& handler, uint8_t *buffer,
		uint8_t length) {
	AirframePtr frame = make_checked<Airframe>();
	frame->setLength(length);
	memcpy(frame->getData(), buffer, length);
	Endpoint::sendRequest(new DataRequest(MAC_BROADCAST, frame));
}
