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

#ifndef ASSOCIATIONSERVICE_H_
#define ASSOCIATIONSERVICE_H_

#include "Endpoint.h"
#include "logging.h"

namespace cometos {

/**
 * This class periodically sends beacon to the base station. Based on this
 * the base station can learn the identifier of the node. By using RMI technique
 * the base station can stop the beaconing of the nodes. The beaconing itself
 * is done with exponential backoffs, meaning after startup beacons are send
 * with a high frequency. Later the frequency falls to a very low value using
 * an  exponential decay.
 */
class AssociationService : public Endpoint{
public:

    static const uint16_t BASE_TIME_DURATION;
    static const uint8_t  MAX_INTERVAL;

    AssociationService(const char * service_name = NULL);

	void initialize();

	void timeout(Message *msg);

	/**Configures interval of association messages. Is RMI
	 * method.
	 *
	 * @param maxInterval if value is zero no association methods are sent*/
	void set(uint8_t &maxInterval, uint8_t &currInterval);

#ifdef OMNETPP
	void finish() {
	    LOG_DEBUG("Cancel ScheduleMsg");
	    cancel(&schdmsg);
	}
#endif

private:
	uint8_t maxInterval;
	uint8_t currInterval;
	uint8_t counter;

	Message schdmsg;
};


} /* namespace cometos */
#endif /* ASSOCIATIONSERVICE_H_ */
