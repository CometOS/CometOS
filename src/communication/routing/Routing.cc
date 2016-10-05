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

/*INCLUDES-------------------------------------------------------------------*/
#include "Routing.h"
#include "logging.h"

#include "palId.h"

using namespace std;
using namespace cometos;

/*METHOD DEFINITION----------------------------------------------------------*/

// this macro allows accessing this module in OMNETPP
Define_Module( Routing);

bool operator==(const packet_id_t &op1, const packet_id_t & op2) {
	return (op1.src == op2.src && op1.seq == op2.seq);
}

Routing::Routing(const char* name) :
		Layer(name), sendingOffset(ROUTING_SENDING_OFFSET), maxHops(
				ROUTING_MAX_HOPS) {
}

void Routing::handleRequest(DataRequest* msg) {
	NwkHeader nwk(msg->dst, palId_id(), ++sequence); // generate network header
	isDuplicate(nwk); // store packet in duplicate list
	handleRequest(msg, nwk); // forward to next processing unit
}

void Routing::finish() {
	RECORD_SCALAR(forwarded);

}

void Routing::handleRequest(DataRequest* msg, NwkHeader& nwk) {
	msg->getAirframe() << nwk; // add network header to packet
	msg->dst = BROADCAST;

	timeOffset_t offset = getFloodingOffset(nwk);

	forwarded++;
	sendRequest(msg, offset);
}

void Routing::handleIndication(DataIndication* msg) {
	NwkHeader nwk;
	msg->getAirframe() >> nwk; // decapsulate header

	if (isDuplicate(nwk)) {
		LOG_DEBUG("DISCARD_DUP pckt from " << nwk.src << " to " <<nwk.dst << " w/ seq=" << (uint16_t)nwk.seq);
		delete msg;
		return;
	}LOG_DEBUG("RECEIVED pckt from " << msg->src);
	handleIndication(msg, nwk);

}

void Routing::handleIndication(DataIndication* pkt, NwkHeader& nwk) {

	nwk.hops++;

	// check whether copy of packet should be sent to upper layer
	if (nwk.dst == palId_id() || nwk.dst == BROADCAST) {
		DataIndication * ind= new DataIndication( pkt->getAirframe().getCopy(), nwk.src, nwk.dst);
		ind->set(new RoutingInfo(nwk.hops));
		sendIndication(ind);

	}
	// forward if node is not target and maximum hop count is not already reached
	if ((nwk.dst != palId_id()) && (nwk.hops <= maxHops)) {
		handleRequest(new DataRequest(nwk.dst, pkt->decapsulateAirframe()),
				nwk); // create request for sending
	}
	delete pkt; // delete indication message
}

void Routing::initialize() {
	Layer::initialize();

	CONFIG_NED(sendingOffset);
	CONFIG_NED(maxHops);

	forwarded = 0;
	sequence = 0;
	schedule(new Message(), &Routing::refresh, ROUTING_HISTORY_REFRESH);
}


void Routing::refresh(Message *timer) {
	history.refresh();
	schedule(timer, &Routing::refresh, ROUTING_HISTORY_REFRESH);
}

bool Routing::isDuplicate(NwkHeader& nwk) {
	LOG_DEBUG("DUPLICATE_CHECK of pckt from " << nwk.src << " w/ seq=" << (uint16_t)nwk.seq);
	return history.checkAndAdd(packet_id_t(nwk.src, nwk.seq));
}

timeOffset_t Routing::getFloodingOffset(NwkHeader& nwk) {
	timeOffset_t offset = 0;
	if (palId_id() != nwk.src && sendingOffset > 0) {
		offset = sendingOffset / 2 + intrand(sendingOffset);
	}
	return offset;
}

