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

#include "MacInterfTest.h"
#include <string.h>

Define_Module(MacInterfTest);

MacInterfTest::MacInterfTest() {

}

void MacInterfTest::initialize() {
	cometos::MacAbstractionLayer::initialize();
	timer = new cometos::Message;
	schedule(timer, &MacInterfTest::traffic);
	i = 1;
	j = 1;
	n = 0;
	max_pkt = par("max_pkt");
	max_nodes = par("max_nodes");
	payload = par("payload");

	row = 0;
	if (0 == strcmp(getParentModule()->getName(), "node2")) {
		row = 1;
	}

	if (getId() == 0 && row == 0) {
		counter = 0;
		counter2 = 0;
		result.open("res.log", std::ios::out);
	}
}

void MacInterfTest::finish() {
	cometos::MacAbstractionLayer::finish();
	cancel(timer);
	delete timer;
	if (getId() == 0 && row == 0) {
		result.close();
	}
}

void MacInterfTest::rxEnd(cometos::AirframePtr frame, node_t src, node_t dst, cometos::MacRxInfo const & info) {
	uint8_t r;
	(*frame) >> r;
	frame.delete_object();
	if (getId() != 0 || row != 0) {
		return;
	}

	if (src == i && r == 0) {
		counter++;
	} else if (src == j && r == 1) {
		counter2++;
	} else {
		ASSERT(false);
		std::cout << "Unexpected packet received" << std::endl;
	}

}

void MacInterfTest::txEnd(cometos::macTxResult_t success, cometos::MacTxInfo const& info) {

}

void MacInterfTest::traffic(cometos::Message* msg) {

	n++;
	if (n > max_pkt) {
		if (getId() == 0 && row == 0) {
			result << (int) i << " " << (int) j << " "
					<< (1.0 * counter) / max_pkt << " "
					<< (1.0 * counter2) / max_pkt << std::endl;
			counter = 0;
			counter2 = 0;
		}
		j++;
		n = 1;
	}

	if (j >= max_nodes) {
		if (getId() == 0) {
			result << std::endl;
		}
		i++;
		j = 1;
	}

	if ((i == getId() && row == 0) || (j == getId() && row == 1)) {
	    cometos::AirframePtr frame = cometos::make_checked<cometos::Airframe>();
		frame->setLength(payload);
		(*frame) << row;
		// send packet immediately (no backoff, no CSMA)
		MacAbstractionLayer::sendAirframe(frame, MAC_BROADCAST, 0);
	}
	if (i < max_nodes) {
		schedule(timer, &MacInterfTest::traffic, 100);
	}

}
