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

#include "MacPacket.h"

namespace cometos {

MacPacket::MacPacket(Airframe *frame) :
	cPacket() {

	if (frame != NULL) {
		//TODO is this still used? semantic of getData changed!
		encapsulateArray(frame->getData(), frame->getLength());
	}

}

MacPacket::~MacPacket() {
}

void MacPacket::encapsulateArray(const uint8_t* data, pktSize_t size) {
	ASSERT(size < AIRFRAME_MAX_SIZE);
	this->size = size;
	setByteLength(size);
	memcpy(this->data, data, size);
}

Airframe *MacPacket::decapsulateAirframe() {
	Airframe *p = new Airframe;
	for (uint8_t i = 0; i < size; i++) {
		(*p) << data[i];
	}
	setByteLength(0);
	return p;
}

MacPacket& MacPacket::operator=(const MacPacket& other) {
	if (this == &other) {
		return *this;
	}

	setName(other.getName());
	setTimestamp(other.getTimestamp());
	cPacket::operator=(other);

	encapsulateArray(other.data, other.size);
	meta = other.meta;

	return *this;
}

MacPacket::MacPacket(const MacPacket& other) {
	operator=(other);
}

//void MacPacket::parsimPack(omnetpp::cCommBuffer *b) {
//	cPacket::parsimPack(b);
//}
//void MacPacket::parsimUnpack(omnetpp::cCommBuffer *b) {
//	cPacket::parsimUnpack(b);
//}

}

