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

#include "UDPComm.h"
#include "TaskScheduler.h"
#include "Airframe.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "MacAbstractionBase.h"
#include "palLed.h"
#include "OverwriteAddrData.h"
#include <stdint.h>
#include "palExec.h"

#include "palId.h"

#define POLL_UDP_INTERVAL 1

using namespace cometos;

const char * const UDPComm::MODULE_NAME = "rsc";

UDPComm::UDPComm(int ownPort, const char* remoteAddress, int remotePort)
: LowerEndpoint(MODULE_NAME),
  ownPort(ownPort),
  dynamicRemote(false),
  remoteAddressInitialized(false),
  taskRx(*this),
  remoteAddressStr(remoteAddress),
  remotePort(remotePort)
{
	rxFrame = cometos::make_checked<cometos::Airframe>();
}

// for dynamic remote
UDPComm::UDPComm(int ownPort)
: UDPComm(ownPort, "", 0)
{
	dynamicRemote = true;
}

void UDPComm::initializeRemoteAddress() {
	if(dynamicRemote) {
		return;
	}

#if defined(_WIN32) || defined(__linux__)
	unsigned long ipAddress; /* internet address */
	if (INADDR_NONE == (ipAddress = lookupHostAddress(remoteAddressStr.c_str()))) {
		cometos::getCout() << "lookupHostAddress() failed" << cometos::endl;
		return;
	}

	/* fill the server address structure */
	memset(&remoteAddr, 0, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(remotePort);
	remoteAddr.sin_addr.s_addr = ipAddress;
#else
    	if(fnet_inet_ptos(remoteAddressStr.c_str(), (struct sockaddr *)&remoteAddr) == FNET_ERR)
    	{
		cometos::getCout() << "Calculating address failed" << cometos::endl;
		return;
    	}

	remoteAddr.sin_port = htons(remotePort);
#endif

	remoteAddressInitialized = true;
}

void UDPComm::initialize() {
	struct sockaddr_in myaddr;      /* our address */

        /* create a UDP socket */
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                cometos::getCout() << "cannot create reset socket" << cometos::endl;
		ASSERT(1 == 2); // wait for the watchdog
                return;
        }

        /* bind the socket to any valid IP address and a specific port */
        memset((char *)&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(ownPort);

        if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
                cometos::getCout() << "bind failed" << cometos::endl;
		ASSERT(1 == 2); // wait for the watchdog
                return;
        }

	palExec_atomicBegin();
	initializeRemoteAddress();
	palExec_atomicEnd();

	cometos::getScheduler().add(taskRx, POLL_UDP_INTERVAL);
}

// RECEPTION ------------------------------------------------------------------

void UDPComm::pollUDPSocket() {
        struct sockaddr_in rxAddr;
#ifdef FNET
        int addrlen = sizeof(rxAddr);
#else
        unsigned int addrlen = sizeof(rxAddr);
#endif

	ASSERT(rxFrame != NULL);

#ifdef FNET
	// FNET is non-blocking anyway!
	#define MSG_DONTWAIT 0
#endif

        int recvlen = recvfrom(fd, (char*)rxFrame->getData(), rxFrame->getMaxLength(), MSG_DONTWAIT, (struct sockaddr *)&rxAddr, &addrlen);
        //cometos::getCout() << "received " << (int16_t)recvlen << " bytes" << cometos::endl;
        if (recvlen > 0) {
		// set length appropriately
		rxFrame->setLength(recvlen);

		node_t src;
		node_t dst;

		ASSERT(rxFrame->getLength()>=sizeof(node_t)*2);
		(*rxFrame) >> src >> dst;

		cometos::DataIndication *ind = new cometos::DataIndication(rxFrame, src, dst);
		cometos::MacRxInfo * phy = new cometos::MacRxInfo(LQI_MAX,
				true,
				cometos::MacRxInfo::RSSI_EMULATED,
				true,
				0);

		ind->set(phy);

		//ind->getAirframe().print();

		sendIndication(ind);

		rxFrame = cometos::make_checked<cometos::Airframe>();

		if(dynamicRemote) {
			ASSERT(sizeof(remoteAddr) == addrlen);
			palExec_atomicBegin();
			memcpy(&remoteAddr, &rxAddr, addrlen);
			remoteAddressInitialized = true;
			palExec_atomicEnd();
		}
        }

	cometos::getScheduler().add(taskRx, POLL_UDP_INTERVAL);
}


// TRANSMISSION ------------------------------------------------------------------

// Is called from upper layer to issue a transmission 
void UDPComm::handleRequest(cometos::DataRequest* msg) {
	//cometos::getCout() << "UDPComm: handleRequest" << cometos::endl;

	struct sockaddr_in txAddr;

	ASSERT(msg->getAirframe().getLength() + sizeof(node_t)*2 <= msg->getAirframe().getMaxLength());
	if (msg->has<OverwriteAddrData>()) {
		OverwriteAddrData * meta = msg->get<OverwriteAddrData>();
		msg->getAirframe() << meta->dst << meta->src;
	}
	else {
		msg->getAirframe() << msg->dst
			<< palId_id();
	}

	palExec_atomicBegin();
	if(!remoteAddressInitialized) {
		initializeRemoteAddress();
	}

	if(!remoteAddressInitialized) {
		palExec_atomicEnd();

		confirm(msg, false, false, false);
		delete msg;
		return;
	}

	memcpy(&txAddr, &remoteAddr, sizeof(txAddr));
	palExec_atomicEnd();

	/* send a message to the server */
	if (sendto(fd, (char*)msg->getAirframe().getData(), msg->getAirframe().getLength(), 0, (struct sockaddr *)&txAddr, sizeof(txAddr)) < 0) {
		cometos::getCout() << "Sending UDP message failed" << cometos::endl;
		confirm(msg, false, false, false);
	}
	else {
		confirm(msg, true, true, true);
	}

	delete msg;
}

// HELPER ------------------------------------------------------------------

void UDPComm::confirm(cometos::DataRequest * req, bool result, bool addTxInfo,
		bool isValidTxTs) {
	cometos::DataResponse * resp = new cometos::DataResponse(result);
	mac_dbm_t rssi;
	if (addTxInfo) {
		if (req->dst == MAC_BROADCAST || !result) {
			rssi = RSSI_INVALID;
		} else {
			rssi = cometos::MacRxInfo::RSSI_EMULATED;
		}
		cometos::MacTxInfo * info = new cometos::MacTxInfo(req->dst, 0, 0,
				rssi, rssi, isValidTxTs, 0);
		resp->set(info);
	}
	req->response(resp);
}

