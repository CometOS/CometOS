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

#include "PredefinedTreeRouting.h"
#include "RoutingInfo.h"
#include "palId.h"
#include "logging.h"

using namespace cometos;
#ifdef OMNETPP
using namespace std;
using namespace omnetpp;
#endif



Define_Module(PredefinedTreeRouting);

#ifdef OMNETPP
bool PredefinedTreeRouting::isInitialized = false;
map<node_t, PredefinedTreeRouting::tree_t> PredefinedTreeRouting::routing;

#endif

PredefinedTreeRouting::PredefinedTreeRouting() :
		parent(0xffff), isSink(false) {
}

PredefinedTreeRouting::PredefinedTreeRouting(node_t parent, bool isSink) :
		parent(parent), isSink(isSink) {
}

void PredefinedTreeRouting::handleRequest(DataRequest* msg) {
	if (isSink) {
		msg->response(new DataResponse(true));
		sendIndication(new DataIndication(msg->decapsulateAirframe(), palId_id(), palId_id()));
		delete msg;
	} else {
		Routing::handleRequest(msg);
	}

}

void PredefinedTreeRouting::initialize() {
	Routing::initialize();

#ifdef OMNETPP
	if (!isInitialized) {
		isInitialized = true;
		// get routing tree file

		cXMLElement *a = par("tree");
		cXMLElementList list = a->getElementsByTagName("node");
		ASSERT(a!=NULL);
		for (cXMLElementList::iterator it = list.begin(); it != list.end();
				it++) {
			int id_, parent_, isSink_;
			sscanf((*it)->getAttribute("id"), "%x", &id_);
			sscanf((*it)->getAttribute("parent"), "%x", &parent_);
			isSink_ = atoi((*it)->getAttribute("isSink"));

			tree_t item;
			item.isSink = isSink_;
			item.parent = parent_;
			routing[id_] = item;
		}
	}
	// load settings from STL container
	parent = routing[getId()].parent;
	isSink = routing[getId()].isSink;
#endif

}

void PredefinedTreeRouting::handleIndication(DataIndication* pkt,
		NwkHeader& nwk) {

	if (isSink) {
		pkt->dst = nwk.dst;
		pkt->src = nwk.src;
		pkt->set(new RoutingInfo(nwk.hops));
		sendIndication(pkt);
	} else {

		handleRequest(
				new DataRequest(nwk.dst, pkt->decapsulateAirframe()),
				nwk); // create request for sending
		delete pkt;
	}
}

void PredefinedTreeRouting::handleRequest(DataRequest* msg, NwkHeader& nwk) {
    nwk.hops++;
	msg->getAirframe() << nwk; // add network header to packet
	if (nwk.dst == BROADCAST) {
		msg->dst = BROADCAST;
	} else {
		msg->dst = parent;
	}
	LOG_INFO("send to "<<msg->dst);


	timeOffset_t offset = 0;

	// predefined tree routing is currently only used for unicast routing
	ASSERT(msg->dst!=BROADCAST);


	if (palId_id() != nwk.src && sendingOffset > 0 && msg->dst == BROADCAST) {
		offset = sendingOffset / 2 + intrand(sendingOffset);
	}

	sendRequest(msg, offset);
}

