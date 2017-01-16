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
 * @author Florian Kauer
 */

#ifndef TRAFFIC_EVALUATION_H_
#define TRAFFIC_EVALUATION_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Endpoint.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class TrafficEvaluation : public cometos::Endpoint {
public:
    TrafficEvaluation(uint8_t msgSize = 60,
                   timeOffset_t meanInterval = 500,
                   time_ms_t warmupDuration = 1000,
                   time_ms_t cooldownDuration = 2000,
                   int16_t maxMeasurementPackets = 100);

    virtual ~TrafficEvaluation();

	/**Sets parameters*/
    void initialize();

    void setDestination(node_t destination);

    void finish();

	void traffic(Message *timer);

	void scheduleResponse(DataResponse *response);
	void handleResponse(DataResponse *response);

	virtual void handleIndication(DataIndication* msg);

private:
    bool destinationSet;
    node_t destination;

	uint64_t counter;
	uint64_t failed;

	uint32_t ts;

	timeOffset_t meanInterval;
	cometos::AirframePtr frame;
	uint16_t myCrc;
	uint8_t msgSize;

    time_ms_t warmupDuration;
    time_ms_t cooldownDuration;

    int64_t sequenceNumber;

    int64_t measurementPackets;
    int16_t maxMeasurementPackets;

    time_ms_t lastHotReception;

#ifdef OMNETPP
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

#endif /* TRAFFIC_EVALUATION_H_ */
