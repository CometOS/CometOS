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

#include "SimpleCsmaMac.h"
#include "logging.h"

namespace cometos {

Define_Module(SimpleCsmaMac);

SimpleCsmaMac::SimpleCsmaMac(const char * name) :
		MacAbstractionLayer(name), gateReqIn(this,
				&SimpleCsmaMac::handleRequest, "gateReqIn"), current(NULL) {
}

void SimpleCsmaMac::finish() {
	MacAbstractionLayer::finish();
	if (current) {
		delete current;
		current = NULL;
	}
}

void SimpleCsmaMac::txEnd(macTxResult_t result, MacTxInfo const & info) {

	ASSERT(current);

	DataResponse * response = new DataResponse(result);
	response->set<MacTxInfo>(new MacTxInfo(info));
	current->response(response);
	delete current;
	current = NULL;
	listen();
}

void SimpleCsmaMac::initialize() {
	MacAbstractionLayer::initialize();
	statsSent = 0;
	statsSucceed = 0;

}
void SimpleCsmaMac::handleRequest(DataRequest* msg) {
	// already processing a packet
	if (current) {
		msg->response(new DataResponse(DataResponseStatus::BUSY));
		delete msg;
		return;
	} else {
		current = msg;
		statsSent++;
		bool result = MacAbstractionLayer::sendAirframe(current->decapsulateAirframe(), current->dst,
				TX_MODE_AUTO_ACK | TX_MODE_BACKOFF | TX_MODE_CCA,msg);
		if (result != true) {
		    msg->response(new DataResponse(DataResponseStatus::FAIL_UNKNOWN));
		    current = NULL;
		    delete msg;
		}
	}

}

void SimpleCsmaMac::rxDropped() {
    LOG_INFO("Pckt dropped");
}

void SimpleCsmaMac::rxEnd(AirframePtr frame, node_t src, node_t dst,
		MacRxInfo const & info) {
    // sending the indication is handled by MacAbstractionLayer
}

}
