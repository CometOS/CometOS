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
#ifndef COMMASSESSMENT_H_
#define COMMASSESSMENT_H_

#include "Endpoint.h"
#include "types.h"

// timout in milli seconds
#define COMM_TIMEOUT	200

class CommAssessmentResults {
public:
	uint8_t finished; // true if experiment is finished
	uint16_t dest; // used destination
	uint8_t payl; // used payload
	uint16_t sent; // number of sent packets
	uint16_t recv; // number of received packets
	uint16_t failed; // number of failed transmissions
	uint16_t timeouts; // number of timeouts during transmissions;
	uint16_t time; // length of experiment in ms
};


namespace cometos {

void serialize(ByteVector& buffer, const CommAssessmentResults& value);
void unserialize(ByteVector& buffer, CommAssessmentResults& value);


/**This class provides functionality for experimental
 * assessment of a communication channel. It is completely
 * controlled via CometOS RMI.
 *
 * Following functionality is provided:
 *
 * <li> reliability measurement
 * <li> throughput measurement
 * <li> round trip time measurement
 *
 */
class CommAssessment: public Endpoint {
public:

	enum {
		CommAssement_RTT = 0, CommAssement_RTT_RESP
	};

	CommAssessment(const char* name = NULL);
	virtual ~CommAssessment();
	void initialize();

	/**Destination address for performing assessment*/
	uint16_t dest;

	/**Size of payload*/
	uint8_t payl;

	virtual void handleIndication(DataIndication* msg);

	virtual void handleResponse(DataResponse *msg);

	virtual void sendRttPacket();

	virtual void responseTimeout(Message* timer);

	void finish();

	void resetStats();

	/**Starts assessment based on round trip time measurement.
	 * Works not in broadcast mode
	 *
	 *@param send number of packets to send
	 */
	void rttAssement(uint16_t &send);

	void rttDone();

	CommAssessmentResults getResults();

	Message* timeoutTimer;

private:
	uint8_t finished;
	uint16_t sent;
	uint16_t recv;
	uint16_t failed;
	uint16_t timeouts;
	uint16_t time;

	time_ms_t startTime;
	uint16_t counter;
};

} /* namespace cometos */
#endif /* COMMASSESSMENT_H_ */
