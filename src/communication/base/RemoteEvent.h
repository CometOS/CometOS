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

#ifndef REMOTEEVENT_H_
#define REMOTEEVENT_H_

#include "types.h"
#include "Airframe.h"


namespace cometos {

class RemoteModule;

class RemoteEventBase {
public:
	~RemoteEventBase() {
	}

	RemoteEventBase(RemoteModule* owner, const char *name);

	bool subscribe(void *ra, node_t dst,uint16_t counter);

	const char * getName();

	void* getAccessModule();

	/**
	 *
	 * @param dst if broadcast all subscribers are removed, otherwise only dest
	 */
	bool unsubscribe( node_t dst);


	void raiseEvent(AirframePtr frame);

	RemoteEventBase *next;

private:
	node_t dst;
	RemoteModule* owner;
	const char *name;
	void *ra;
	uint16_t counter;
};


template<class T>
class RemoteEvent: public RemoteEventBase {
public:

	 RemoteEvent(RemoteModule* owner, const char *name):
		 RemoteEventBase(owner,name) {}

	void raiseEvent(T& value) {
		if (getAccessModule()==NULL) {
			return;
		}

		AirframePtr air = make_checked<Airframe>();
		(*air) << value;
		RemoteEventBase::raiseEvent(air);
	}
};

} /* namespace cometos */
#endif /* REMOTEEVENT_H_ */
