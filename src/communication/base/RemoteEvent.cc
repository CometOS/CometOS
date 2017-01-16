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

#include "RemoteEvent.h"
#include "RemoteModule.h"
#include "RemoteAccess.h"
#include "AirString.h"

namespace cometos {

RemoteEventBase::RemoteEventBase(RemoteModule* owner, const char *name) :
		dst(BROADCAST), owner(owner), name(name), ra(NULL), counter(0) {
	next = owner->nextEvent;
	owner->nextEvent = this;
}

bool RemoteEventBase::subscribe(void *ra, node_t dst, uint16_t counter) {
	if (dst == BROADCAST || ra == NULL || counter == 0) {
		return false;
	}

	this->counter = counter;
	this->ra = ra;
	this->dst = dst;
	return true;
}

bool RemoteEventBase::unsubscribe(node_t dst) {
	if (dst != BROADCAST && this->dst != dst) {
		return false;
	}
	this->counter = 0;
	this->ra = NULL;
	this->dst = BROADCAST;
	return true;
}

void RemoteEventBase::raiseEvent(AirframePtr frame) {
	if (ra == NULL) {
		return;
	}

	if (counter == 0) {
		this->ra = NULL;
		this->dst = BROADCAST;
		return;
	}

	 AirString strName( name);
	 AirString strOwner(owner->getName());

	(*frame) << strName<< strOwner << counter;
	((RemoteAccess*) ra)->raise(frame, dst);
#ifdef OMNETPP
	// similiar to Enter_Method_Silent();
omnetpp::cMethodCallContextSwitcher __ctx(owner);
	__ctx.methodCallSilent();
#endif
	counter--;
}

const char * RemoteEventBase::getName() {
    return name;
}

void * RemoteEventBase::getAccessModule() {
    return ra;
}

} /* namespace cometos */
