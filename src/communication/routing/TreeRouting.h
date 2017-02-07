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
 * @author Stefan Unterschuetz
 */

#ifndef TREEROUTING_H_
#define TREEROUTING_H_

#include "Routing.h"
#include "DuplicateFilter.h"

namespace cometos {

/**Simple self-stabilizing/pro-active tree routing approach.
 * This algorithm uses separate gate for beacons
 */
class TreeRouting: public Routing {
public:

	TreeRouting();

	void initialize();

	void finish();

	void handleRequest(DataRequest* msg);

	void handleIndication(DataIndication* pkt, NwkHeader& nwk);

	void handleBeacon(DataIndication* pkt);

	void handleRequest(DataRequest* msg, NwkHeader& nwk);

	virtual void sendBeacon(Message *timer);

	void slotTimeout(Message *timer);

	InputGate<DataIndication>  beaconIn;

	OutputGate<DataRequest> beaconOut;

	bool isSink;

protected:


	uint16_t slotDuration;

	node_t parent;
	uint8_t hops;

	node_t nextParent;
	uint8_t nextHops;




	uint16_t slotsToRun;

#ifdef ROUTING_ENABLE_STATS
    uint16_t forwarded;
    uint16_t control;
    uint16_t numControlRecv;
#endif

};

} /* namespace cometos */
#endif /* TREEROUTING_H_ */
