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
 * @author Andreas Weigel
 */
#include "FwdBounce.h"
#include "OverwriteAddrData.h"

using namespace cometos;

FwdBounce::FwdBounce(const char * name, int interval, bool debug) :
	cometos::Endpoint(name),
	fromSecondary(this, &FwdBounce::handleIndicationFromSecondary, "fromSecondary"),
	toSecondary(this, "toSecondary"),
	interval(interval),
	secondaryToPrimary(0),
	primaryToSecondary(0),
	debug(debug)
{

}


void FwdBounce::initialize() {
	cometos::Endpoint::initialize();
	if (interval > 0) {
		schedule(&timer, &FwdBounce::timerFired, interval);
	}
}


void FwdBounce::handleIndicationFromSecondary(cometos::DataIndication * msg) {
	if (debug) {
		getCout() << "t->s: ";
	}
	secondaryToPrimary++;
	handle(msg, gateReqOut);
}


void FwdBounce::handleIndication(cometos::DataIndication * msg) {
	if (debug) {
		getCout() << "s->t: ";
	}
	primaryToSecondary++;
	handle(msg, toSecondary);
}


void FwdBounce::timerFired(cometos::Message * msg) {
	getCout() << "secondaryToPrimary: " << secondaryToPrimary << "  |  primaryToSecondary: " << primaryToSecondary << cometos::endl;
	schedule(msg, &FwdBounce::timerFired, interval);
}

void FwdBounce::recvPkt(ByteVector& frame, int src) {
}


void FwdBounce::handle(cometos::DataIndication * msg, cometos::OutputGate<cometos::DataRequest> & outputGate) {
	if (debug) {
		cometos::Airframe &frame = msg->getAirframe();
        /*
		std::cout << std::hex << std::uppercase << right;
		std::cout.fill('0');
		std::cout << "len= " << (int)frame.getLength();
		for (int i = 0; i < frame.getLength(); i++) {
		    if (i % 16 == 0) {
                std::cout << std::endl << " ";
            }
			std::cout << "0x";
			std::cout.width(2);
			std::cout << (int) frame.getData()[i] << " ";
		}
		std::cout << std::dec << std::endl;
		std::cout.unsetf(std::ios_base::uppercase);
        */
        frame.printFrame();
	}

	recvPkt(msg->getAirframe().getArray(), msg->src);

	OverwriteAddrData * meta = new OverwriteAddrData(msg->src, msg->dst);
	cometos::DataRequest * req = new cometos::DataRequest(msg->dst, msg->decapsulateAirframe());
	delete(msg);
	req->set(meta);
	outputGate.send(req);
}
