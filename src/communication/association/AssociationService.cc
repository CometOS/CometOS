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

#include "AssociationService.h"

#ifndef BASESTATION_ADDR
#define BASESTATION_ADDR 0
#endif

namespace cometos {
Define_Module(AssociationService);

const uint16_t AssociationService::BASE_TIME_DURATION = 10000;
const uint8_t  AssociationService::MAX_INTERVAL = 255;

AssociationService::AssociationService(const char * service_name) :
        Endpoint(service_name),
        maxInterval(MAX_INTERVAL),
        currInterval(1),
        counter(0)
{}

void AssociationService::initialize() {
	Endpoint::initialize();

	schedule(&schdmsg, &AssociationService::timeout,
			BASE_TIME_DURATION / 2 + intrand(BASE_TIME_DURATION));

	remoteDeclare(&AssociationService::set, "set");
}

void AssociationService::timeout(Message *msg) {
	schedule(msg, &AssociationService::timeout,
			BASE_TIME_DURATION / 2 +intrand(BASE_TIME_DURATION));

	if (this->maxInterval == 0) {
		return;
	}

	this->counter++;
	if (this->counter >= this->currInterval) {
		this->counter = 0;

		if ((uint8_t) (this->currInterval * 2) > this->maxInterval) {
			this->currInterval = this->maxInterval;
		} else {
			this->currInterval *= 2;
		}
		sendRequest(new DataRequest(BASESTATION_ADDR, make_checked<Airframe>()));

	}
}

void AssociationService::set(uint8_t &maxInterval, uint8_t &currInterval) {
	this->maxInterval = maxInterval;
	this->currInterval = currInterval;
	this->counter = 0;
}

} /* namespace cometos */
