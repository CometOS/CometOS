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

#ifndef MAC_PACKET_H_
#define MAC_PACKET_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "Airframe.h"
#include "ObjectContainer.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {
/**
 * This class adds a data array to the message class used for sending data to other wireless nodes
 */
class MacPacket: public omnetpp::cPacket {
public:

	virtual ~MacPacket();

	Airframe *decapsulateAirframe();

	void encapsulateArray(const uint8_t* data, pktSize_t size);

	MacPacket(Airframe* frame = NULL);

	MacPacket& operator=(const MacPacket& other);

	MacPacket(const MacPacket& other);

	virtual MacPacket *dup() const {
		return new MacPacket(*this);
	}

//	virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
//	virtual void parsimUnpack(omnetpp::cCommBuffer *b);

	uint8_t data[AIRFRAME_MAX_SIZE];
	pktSize_t size;

	// optional meta data
	ObjectContainer meta;
};

inline void doPacking(omnetpp::cCommBuffer *b, MacPacket& obj) {
	obj.parsimPack(b);
}

inline void doUnpacking(omnetpp::cCommBuffer *b, MacPacket& obj) {
	obj.parsimUnpack(b);
}

}

#endif /* MAC_PACKET_H_ */
