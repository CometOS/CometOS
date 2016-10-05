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

#ifndef PERIODICTRAFFIC_H_
#define PERIODICTRAFFIC_H_

#include "Endpoint.h"

#define TRAFFIC_MAX_PAYLOAD		20

namespace cometos {

class PeriodicTraffic: public cometos::Endpoint {
public:
	/**Sets parameters*/
	void initialize();

	/**Periodically generates traffic
	 */
	void traffic(Message *timer);

	void finish();

	/**Handles response message*/
	void resp(DataResponse *response);

	void handleIndication(DataIndication* msg);

	/**Debuugin Method for RMI*/
	StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> get(uint8_t& length, uint8_t& start);

	void start(timeOffset_t offset);

	int sendCounter;
	int receiveCounter;

	node_t dst;
	timeOffset_t interval;
	pktSize_t payloadSize;
};
/// additional serialization routines


uint8_t getTrafficPayload(StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list, uint8_t index);


void serialize(ByteVector & buf, const StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list);
void unserialize(ByteVector & buf, StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list);




} /* namespace cometos */
#endif /* PERIODICTRAFFIC_H_ */
