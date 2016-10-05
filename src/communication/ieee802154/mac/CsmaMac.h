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
 * @author Stefan Unterschuetz, Andreas Weigel
 */


#ifndef CSMA_MAC_H_
#define CSMA_MAC_H_

#include "MacAbstractionLayer.h"
#include "MacAbstractionBase.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "SList.h"
#include "MacStats.h"

namespace cometos {

#define MAC_MODULE_NAME "mac"

enum {
	CSMA_MAC_QUEUE_LENGTH = 5
};



/**
 * An simple CSMA Mac layer with queue and support for basic stats recording.
 */
class CsmaMac: public MacAbstractionLayer {
public:
	CsmaMac(const char* name = NULL,
	        const node_t* myAddress = NULL);

	void initialize();

	virtual void finish();

	virtual void rxEnd(Airframe *frame, node_t src, node_t dst, MacRxInfo const & info);

	virtual void txEnd(macTxResult_t success, MacTxInfo const & info);

	virtual void handleRequest(DataRequest* msg);
	
	virtual void rxDropped();



	/**
	 * Sends DataIndication message to connected module.
	 *
	 * @param request	valid pointer to DataIndication object (ownership is passed)
	 * @param ms		sending offset in milliseconds
	 */
//	void sendIndication(DataIndication* indication, timeOffset_t offset=0);

	virtual void sendNext();

#ifdef MAC_ENABLE_STATS
    MacStats getStats();
    void resetStats();
#endif

	InputGate<DataRequest> gateReqIn;

private:
	typedef StaticSList<DataRequest*, CSMA_MAC_QUEUE_LENGTH> PacketQueue;
	PacketQueue queue;

	// statistics collection
#ifdef MAC_ENABLE_STATS
#define MAC_STATS_VAR macStats
	MacStats MAC_STATS_VAR;

	#define MAC_STATS_INIT(x) (MAC_STATS_VAR.x) = 0
	#define MAC_STATS_INC(x) (MAC_STATS_VAR.x)++
	#define MAC_STATS_ADD(stat, value) ((MAC_STATS_VAR.stat) += (value))
	#define MAC_STATS_RECORD(x) recordScalar((#x), (MAC_STATS_VAR.x))

#ifdef OMNETPP
	omnetpp::cOutVector queueLevel;
    #define MAC_STATS_NAME_VECTOR(vec) (vec).setName(#vec);
	#define MAC_STATS_TO_VECTOR(vec, value) (vec).record((value))
    #define MAC_STATS_TO_VECTOR_WITH_TIME(vec, value) (vec).recordWithTimestamp(omnetpp::simTime(), (value))
#else
	#define MAC_STATS_NAME_VECTOR(vec)
	#define MAC_STATS_TO_VECTOR(vec, value)
	#define MAC_STATS_TO_VECTOR_WITH_TIME(vec, value)
#endif // OMNETPP
#else
	#define MAC_STATS_NAME_VECTOR(vec)
	#define MAC_STATS_TO_VECTOR(vec, value)
	#define MAC_STATS_TO_VECTOR_WITH_TIME(vec, value)
	#define MAC_STATS_INIT(x)
	#define MAC_STATS_INC(x)
	#define MAC_STATS_ADD(stat, value)
	#define MAC_STATS_RECORD(x)
#endif // MAC_ENABLE_STATS
};

} 

#endif /* CSMACAMAC_H_ */
