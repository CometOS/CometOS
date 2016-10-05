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

#include "NetworkInterfaceSwitch.h"
#include "logging.h"
#include "types.h"
#include "MacAbstractionBase.h"
#include "palId.h"

Define_Module(NetworkInterfaceSwitch);

NetworkInterfaceSwitch::NetworkInterfaceSwitch(
			const char* name,
			node_t cniAddrBegin,
			node_t cniAddrEnd) :
		LowerEndpoint(name),
		cniIndIn(this, &NetworkInterfaceSwitch::handleIndication, "cniIndIn"),
		cniReqOut(this, "cniReqOut"),
		nniIndIn(this, &NetworkInterfaceSwitch::handleIndication, "nniIndIn"),
		nniReqOut(this, "nniReqOut"),
		cniAddrBegin(cniAddrBegin),
		cniAddrEnd(cniAddrEnd)
{}

void NetworkInterfaceSwitch::initialize() {
	LowerEndpoint::initialize();
	CONFIG_NED(cniAddrBegin);
	CONFIG_NED(cniAddrEnd);
	LOG_DEBUG("Init NIfS w/ cmmn net addr space [ "<<cniAddrBegin<<" .. "<<cniAddrEnd << "]");
}

void NetworkInterfaceSwitch::handleRequest(cometos::DataRequest* msg) {
	msg->set(createAddrMetaData(msg));

	if (msg->dst == MAC_BROADCAST) {
	    cometos::DataRequest * msg2 = new cometos::DataRequest(msg->dst,
				msg->getAirframe().getCopy());
		msg2->set(createAddrMetaData(msg));
		cniReqOut.send(msg);
		nniReqOut.send(msg2);
		LOG_DEBUG ("brdcst mac pckt");

	} else if ((msg->dst >= cniAddrBegin && msg->dst <= cniAddrEnd)) {
		LOG_DEBUG ("send mac pckt w/ dst Addr. " << msg->dst << " via CNI");
		cniReqOut.send(msg);
	} else {
		LOG_DEBUG ("send mac pckt w/ dst Addr. " << msg->dst << " via NNI");
		nniReqOut.send(msg);
	}
}

void NetworkInterfaceSwitch::handleIndication(cometos::DataIndication* msg) {
	sendIndication(msg);
}

// TODO this method contains not very elegant and/or beautiful means of retrieving
// addresses
OverwriteAddrData * NetworkInterfaceSwitch::createAddrMetaData(
		const cometos::DataRequest * const msg) {
	OverwriteAddrData * oaf;
	cometos::MacAbstractionBase * mac = (cometos::MacAbstractionBase *) getModule(
			MAC_MODULE_NAME);
	if (mac != NULL) {
		oaf = new OverwriteAddrData(mac->getShortAddr(), msg->dst);
	} else {
		node_t src;
		src = palId_id();
		oaf = new OverwriteAddrData(src, msg->dst);
	}
	return oaf;

}
