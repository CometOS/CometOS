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

/**This file declares classes used for the traffic example.
 * @author Stefan Unterschütz
 */

#ifndef TRAFFIC_EXAMPLE_H_
#define TRAFFIC_EXAMPLE_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Endpoint.h"
#include "SList.h"
#ifdef OMNETPP
#include <map>
#endif

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class TrafficExample : public cometos::Endpoint {
public:
    TrafficExample(StaticSList<node_t, 10> destAddresses = StaticSList<node_t, 10>(),
                   uint8_t msgSize = 24,
                   timeOffset_t fixedInterval = 500,
                   timeOffset_t rndInterval = 1000,
                   bool snoop = false);

	/**Sets parameters*/
    void initialize();

    void finish();

	void traffic(Message *timer);

	void resp(DataResponse *response);

	virtual void handleIndication(DataIndication* msg);

	void handleSnoopIndication(DataIndication* msg);

	InputGate<DataIndication> gateSnoopIn;

private:
	uint64_t counter;
	uint64_t failed;

	uint32_t ts;

	timeOffset_t fI;
	timeOffset_t rI;
	StaticSList<node_t, 10> destAddresses;
	cometos::Airframe * frame;
	uint16_t myCrc;
	uint8_t msgSize;

	bool snoop;

    int64_t sequenceNumber; // TODO data type

#ifdef OMNETPP
    int timeLimitMS;

	class Reception {
	public:
	    Reception()
	    : corrupted(0),
	      uniques(0),
	      duplicates(0),
	      lastSeqNum(-1)
	    {
	    }

	    unsigned int corrupted;
	    unsigned int uniques;
	    unsigned int duplicates;
	    int64_t lastSeqNum;
	};

	std::map<node_t, Reception> receptions;
#endif
};

} // namespace cometos

#endif /* TRAFFIC_EXAMPLE_H_ */
