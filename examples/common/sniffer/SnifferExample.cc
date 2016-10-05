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

#include "SnifferExample.h"
#include "logging.h"

Define_Module(SnifferExample);

SnifferExample::SnifferExample(const char* name, const char* macName) :
		cometos::Layer(name), macName(macName) {

}

void SnifferExample::initialize() {
	Layer::initialize();

	// get pointer to MAC layer
	mac = dynamic_cast<cometos::MacAbstractionBase*>(getModule(macName));

	// use this if no RTTI is available
	//mac = static_cast<MacAbstractionBase*> (getModule(macName));
    ASSERT(mac!=NULL);

	// set MAC layer to promiscuous mode
	mac->setPromiscuousMode(true);
}

void SnifferExample::handleRequest(cometos::DataRequest* msg) {
	sendRequest(msg);
}

void SnifferExample::handleIndication(cometos::DataIndication* msg) {
	LOG_INFO("receive from "<<msg->src<<" to "<<msg->dst);

	// check if message is for this node
	if (msg->dst != getId() && msg->dst != BROADCAST) {
		delete msg;
	} else {
		sendIndication(msg);
	}
}

