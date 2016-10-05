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
 * @author Stefan Unterschütz
 */

#include "Airframe.h"
#include <string.h>

namespace cometos {

Airframe* Airframe::getCopy() const {
	Airframe *msg = new Airframe;
	msg->data = this->data; // copy
	return msg;
}

Airframe* Airframe::getDeepCopy() const {
    Airframe *msg = new Airframe;
    msg->data = this->data; // copy

    ((ObjectContainer*)msg)->operator=(*this);

    return msg;
}

/*
 Airframe& Airframe::operator=( const Airframe& other) {
 if (this == &other) {
 return *this;
 }

 #ifdef OMNETPP
 setName(other.getName());
 setTimestamp(other.getTimestamp());
 cAirframe::operator=(other);
 #endif
 Message::operator=(other);


 this->pData =  Airframe_MAX_SIZE;

 this->owner = other.owner;

 this->id = other.id;

 encapsulate(&other.data[other.pData], other.getLength());

 return *this;
 }*/

//#ifdef OMNETPP
//Airframe::Airframe(const Airframe& other) {
//	operator=(other);
//}
/*
 void Airframe::parsimPack(cCommBuffer *b) {
 cAirframe::parsimPack(b);
 doPacking(b, this->data,  Airframe_MAX_SIZE	);
 }
 void Airframe::parsimUnpack(cCommBuffer *b) {
 cAirframe::parsimUnpack(b);
 doUnpacking(b, this->data,  Airframe_MAX_SIZE	);
 }

 */
/*
 int Airframe::getNodeId() {
 cModule *module = (cSimulation::getActiveSimulation())->getContextModule();
 while (module != NULL) {
 if (module->hasPar("id")) {
 return (int) module->par("id");
 }
 module = module->getParentModule();
 }
 return -1;
 }
 */

} /* namespace cometos */

