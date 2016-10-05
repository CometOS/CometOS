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
#include "BsBridge.h"
#include <iostream>

using namespace cometos;

BsBridge::BsBridge(const char * name) :
	cometos::Module(name),
	fromAir(this, &BsBridge::handleIndicationFromAir, "fromAir"),
	toAir(this, "toAir"),
	fromSerial(this, &BsBridge::handleIndicationFromSerial, "fromSerial"),
	toSerial(this, "toSerial"),
	serialToAir(0),
	airToSerial(0)
{

}


void BsBridge::initialize() {
	cometos::Module::initialize();
}


void BsBridge::handleIndicationFromSerial(cometos::DataIndication * msg) {
	serialToAir++;
//	handle(msg, toAir);

}


void BsBridge::handleIndicationFromAir(cometos::DataIndication * msg) {
	airToSerial++;
	OverwriteAddrData * meta = new OverwriteAddrData(msg->src, msg->dst);
    cometos::DataRequest * req = new cometos::DataRequest(msg->dst, msg->decapsulateAirframe());
    delete(msg);
    req->set(meta);
    toSerial.send(req);
}


void BsBridge::recvPkt(ByteVector& frame, int src) {
}


void BsBridge::handle(cometos::DataIndication * msg, cometos::OutputGate<cometos::DataRequest> & outputGate) {

	recvPkt(msg->getAirframe().getArray(), msg->src);

	OverwriteAddrData * meta = new OverwriteAddrData(msg->src, msg->dst);
	cometos::DataRequest * req = new cometos::DataRequest(msg->dst, msg->decapsulateAirframe());
	delete(msg);
	req->set(meta);
	outputGate.send(req);
}
