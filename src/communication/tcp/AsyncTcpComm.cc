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
 * @author Stefan Unterschuetz
 */

#include <iostream>
#include "AsyncTcpComm.h"

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "NwkHeader.h"
#include "MacAbstractionBase.h"
#include "OverwriteAddrData.h"
#include "palId.h"
#include "logging.h"

using namespace std;
using namespace cometos;

Define_Module( AsyncTcpComm);

void AsyncTcpComm::receivedCallback(const socket_t& handler, uint8_t *buffer,
		uint8_t length) {
#ifdef OMNETPP
	Enter_Method_Silent();
#endif


	Airframe *frame = new Airframe();
	frame->setLength(length);
	memcpy(frame->getData(), buffer, length);
	node_t src;
	node_t dst;
	(*frame) >> src >> dst;

	// attach perfect LQI and RSSI values
	MacRxInfo * ppi = new MacRxInfo(LQI_MAX,
			MacRxInfo::RSSI_EMULATED);

	DataIndication * ind = new DataIndication(frame, src, dst);

	ind->set(ppi);
	LOG_DEBUG("fromTCP: dst=" << dst << "|src=" << src);
	//std::cout << "fromTCP: dst=" << dst << "|src=" << src << endl;
	LowerEndpoint::sendIndication(ind);
}

void AsyncTcpComm::handleRequest(cometos::DataRequest* msg) {
	// add src and destination addresses to frame
	// TODO could add well-defined header struct
	node_t desti;
	node_t srci;
	if (msg->has<OverwriteAddrData>()) {
		OverwriteAddrData * meta = msg->get<OverwriteAddrData>();
		desti = meta->dst;
		srci = meta->src;
	} else {
		desti = msg->dst;
		srci = getAddr();
	}
	msg->getAirframe() << desti << srci;

	if (conn.begin() == conn.end()) {
		LOG_DEBUG("TCP not connected; response false");
		msg->response(new DataResponse(false));
		delete (msg);
		return;
	}

	AsyncTcpAgent::sendToAll(msg->getAirframe().getData(),
			msg->getAirframe().getLength());

//	std::cout << "toTCP: dst=" << desti << "|src=" << srci << endl;
	MacTxInfo * mti = new MacTxInfo(desti, 0, MacRxInfo::RSSI_EMULATED,
			MacRxInfo::RSSI_EMULATED);
	DataResponse * response = new DataResponse(true);
	response->set(mti);
	msg->response(response);
	LOG_DEBUG("toTCP: dst=" << desti << "|src=" << srci << "; confirm sent to " << msg->getRequestId());
	delete msg;

	return;

}

#ifdef OMNETPP
void AsyncTcpComm::connect(cometos::Message* msg) {
	delete msg;

	const char *conn = par("conn");
	size_t len = strlen(conn);
	if (len < 1) {
		return;
	}

	char tmp[len];
	strcpy(tmp, conn);
	char *token;

	token = strtok(tmp, ",");
	while (token) {
		char* ip = token;
		while ((*token != ':') && (*token != 0)) {
			token++;
		}
		if (*token == 0) {
			cout << "no port is given for entry " << ip << std::endl;
			return;
		}
		*token = 0;
		token++;

		AsyncTcpAgent::connect(ip, atoi(token));

		token = strtok(NULL, ",");
	}

}
#endif

AsyncTcpComm::AsyncTcpComm(node_t fallbackAddr)
{
}

void AsyncTcpComm::initialize() {
	LowerEndpoint::initialize();
#ifdef OMNETPP

	int portListen = par("listen");

	// start TCP server is port is given
	if (portListen != -1) {
		listen(portListen);
	}
	schedule(new cometos::Message, &AsyncTcpComm::connect);
#endif
}

void AsyncTcpComm::finish() {
	shutdown();
}

inline node_t AsyncTcpComm::getAddr() {
	return palId_id();
}
