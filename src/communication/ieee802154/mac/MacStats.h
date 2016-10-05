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

#ifndef MACSTATS_H_
#define MACSTATS_H_

#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "Vector.h"
#define MAC_STATS_MAX_RETRIES 7

#if MAC_STATS_MAX_RETRIES < MAC_DEFAULT_FRAME_RETRIES && defined MAC_ENABLE_STATS
#error "MAC_ENABLE_STATS is defined but MAC_DEFAULT_FRAME_RETRIES is larger than MAC_STATS_MAX_RETRIES; would causes assertions!"
#endif

namespace cometos {

enum {
	MCT_MIN_BACKOFF,
	MCT_MAX_BACKOFF,
	MCT_MAX_CCA_RETRIES,
	MCT_MAX_RETRIES
};
typedef uint8_t macCfgType_t;


class MacStats {
public:
	MacStats() :
		numPacketsDroppedQueue(0),
		numIncomingPacketsDropped(0),
		numOutgoingPacketsNotAcked(0),
		numOutgoingPacketsAcked(0),
		numOutgoingPackets(0),
		numPacketRetries(0),
		numCCARetries(0),
		numCSMAFails(0)
	{
	    for (int i=0; i <= MAC_STATS_MAX_RETRIES; i++) {
            retryCounter.pushBack(0);
        }
	}

	void reset() {
		numPacketsDroppedQueue = 0;
		numIncomingPacketsDropped= 0;
		numOutgoingPacketsNotAcked= 0;
		numOutgoingPacketsAcked= 0;
		numOutgoingPackets= 0;
		numPacketRetries= 0;
		numCCARetries = 0;
		numCSMAFails = 0;
		for (int i=0; i <= MAC_STATS_MAX_RETRIES; i++) {
            retryCounter[i] = 0;
        }
	}

	uint32_t numPacketsDroppedQueue;
	uint32_t numIncomingPacketsDropped;
	uint32_t numOutgoingPacketsNotAcked;
	uint32_t numOutgoingPacketsAcked;
	uint32_t numOutgoingPackets;
	uint32_t numPacketRetries;
	uint32_t numCCARetries;
	uint32_t numCSMAFails;
	Vector<uint32_t, MAC_STATS_MAX_RETRIES+1> retryCounter;
};



void serialize(ByteVector & buf, MacStats const & val);
void unserialize(ByteVector & buf, MacStats & val);

}

#endif 
