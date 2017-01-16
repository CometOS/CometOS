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
 * @author Stefan Unterschuetz, Andreas Weigel
 */

#include "CsmaMac.h"

#if defined OMNETPP and defined MAC_ENABLE_STATS
#include <string.h>
#endif

#include "palId.h"
#include "palLed.h"

namespace cometos {

Define_Module(CsmaMac);

CsmaMac::CsmaMac(const char * name,
                 const node_t* myAddress) :
		MacAbstractionLayer(name, myAddress),
		gateReqIn(this, &CsmaMac::handleRequest, "gateReqIn") {
}


void CsmaMac::initialize() {
	cometos::MacAbstractionLayer::initialize();
	MAC_STATS_INIT(numIncomingPacketsDropped);
	MAC_STATS_INIT(numPacketsDroppedQueue);
	MAC_STATS_INIT(numOutgoingPacketsNotAcked);
	MAC_STATS_INIT(numOutgoingPackets);
	MAC_STATS_INIT(numOutgoingPacketsAcked);
	MAC_STATS_INIT(numPacketRetries);
	MAC_STATS_INIT(numCCARetries);
	MAC_STATS_INIT(numCSMAFails);
	MAC_STATS_NAME_VECTOR(queueLevel);

#ifdef MAC_ENABLE_STATS
	remoteDeclare(&CsmaMac::getStats, "gs");
	remoteDeclare(&CsmaMac::resetStats, "rs");
#endif
}

void CsmaMac::finish() {
	MacAbstractionLayer::finish();
	while (!queue.empty()) {
		delete queue[queue.begin()];
		queue.pop_front();
	}
	MAC_STATS_RECORD(numIncomingPacketsDropped);
	MAC_STATS_RECORD(numPacketsDroppedQueue);
	MAC_STATS_RECORD(numOutgoingPacketsNotAcked);
	MAC_STATS_RECORD(numOutgoingPackets);
	MAC_STATS_RECORD(numOutgoingPacketsAcked);
	MAC_STATS_RECORD(numPacketRetries);
	MAC_STATS_RECORD(numCCARetries);
	MAC_STATS_RECORD(numCSMAFails);

#if defined OMNETPP && defined MAC_ENABLE_STATS
	std::string retryStr("numPacketsWithRetries");
	for (int i = 0; i <= MAC_STATS_MAX_RETRIES; i++) {
	    std::stringstream ss;
	    if (i / 10 > 0) {
	        ss << retryStr << (char) (i / 10 + 48) << (char) (i % 10 + 48);
	    } else {
	        ss << retryStr << (char) (i + 48);
	    }
	    recordScalar(ss.str().c_str(), macStats.retryCounter[i]);
	}
#endif
}

void CsmaMac::rxEnd(cometos::AirframePtr frame, node_t src, node_t dst,
		MacRxInfo const & info) {
	LOG_DEBUG("info.tsValid=" << info.tsInfo.isValid << "|info.ts=" << info.tsInfo.ts);
}

void CsmaMac::txEnd(macTxResult_t result, MacTxInfo const & info) {

	ASSERT(!queue.empty());

	DataRequest* request = queue[queue.begin()];
	queue.pop_front();
	DataResponse * response = new DataResponse(result);
	response->set<MacTxInfo>(new MacTxInfo(info));
	request->response(response);
	delete request;

	if (result == MTR_CHANNEL_ACCESS_FAIL) {
	    MAC_STATS_INC(numCSMAFails);
	}

	// log information about sending process
	if (info.destination != MAC_BROADCAST) {
		if (result == MTR_SUCCESS) {
			MAC_STATS_INC(numOutgoingPacketsAcked);
		} else {
			MAC_STATS_INC(numOutgoingPacketsNotAcked);
		}
	}
	MAC_STATS_ADD(numCCARetries, info.numCCARetries);
	MAC_STATS_ADD(numPacketRetries, info.numRetries);
	MAC_STATS_TO_VECTOR(queueLevel, queue.size());

#if defined MAC_ENABLE_STATS
	ASSERT(info.numRetries >= 0 && info.numRetries <= MAC_STATS_MAX_RETRIES);
	(macStats.retryCounter[info.numRetries])++;
#endif

	// reactivate MAL and send next packet if there is one
	listen();

	sendNext();
}

void CsmaMac::rxDropped() {
	MAC_STATS_INC(numIncomingPacketsDropped);
}

void CsmaMac::sendNext() {
	if (queue.empty()) {
		return;
	}

	DataRequest* request = queue[queue.begin()];
	bool result;
	result = sendAirframe(request->decapsulateAirframe(), request->dst,
			TX_MODE_AUTO_ACK | TX_MODE_BACKOFF | TX_MODE_CCA, request);

	if (result != true) {
		queue.pop_front();
		DataResponse * response = new DataResponse(DataResponseStatus::FAIL_UNKNOWN);
		response->set<MacTxInfo>(new MacTxInfo(request->dst));
		request->response(response);
		delete request;

		sendNext();
	}
}

void CsmaMac::handleRequest(DataRequest* msg) {
	MAC_STATS_INC(numOutgoingPackets);

	// queue packet
	if (queue.end() == queue.push_back(msg)) {
		msg->response(new cometos::DataResponse(DataResponseStatus::QUEUE_FULL));
		delete msg;
		LOG_WARN("OVERFLOW in msg queue, discarding msg"); MAC_STATS_INC(numPacketsDroppedQueue);
		return;
	}

	LOG_DEBUG("ENQUEUED new msg in mac queue, pos=" << (int) queue.size() - 1); MAC_STATS_TO_VECTOR_WITH_TIME(queueLevel, queue.size());
	// start sending
	if (queue.size() == 1) {
		sendNext();
	}
}

//void CsmaMac::sendIndication(DataIndication * indication, timeOffset_t offset) {
//	gateIndOut.send(indication, offset);
//}

#ifdef MAC_ENABLE_STATS
MacStats CsmaMac::getStats() {
	return MAC_STATS_VAR;
}

void CsmaMac::resetStats() {
	MAC_STATS_VAR.reset();
	for (int i=0; i < MAC_STATS_MAX_RETRIES; i++) {
	    macStats.retryCounter[i] = 0;
	}
}
#endif

}

