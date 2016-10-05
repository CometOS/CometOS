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


#ifndef MESSAGE_H_
#define MESSAGE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include "ObjectContainer.h"
#include "List.h"
#include "Gate.h"
#include "types.h"
#include "Delegate.h"
#include "Event.h"


/*MACROS---------------------------------------------------------------------*/

#define MESSAGE_INVALID_GATE	NULL
#define MESSAGE_SIZE_LOG		20

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

/**
 * Instances of this class are used for exchanging information among
 * services. It derives from UniqueObjectContainer in order to append
 * arbitrary data to this class. In OMNeT++ this class derives from
 * cPacket. However, using methods of cPacket is not recommend.
 */
class Message: public omnetpp::cPacket, public ObjectContainer {
public:

	Message();

	Message& operator=(const Message &msg) {
		ObjectContainer::operator=(const_cast<Message &>(msg));
		return *this;
	}

    Event* getEvent() {
        return &event;
    }

	virtual ~Message();

	BaseInputGate * getInputGate() {
		return inputGate;
	}

	// pointer handler for message
	BaseInputGate *inputGate;

	// Delegate concept currently only partly integrated in OMNeT++
	UnboundedDelegate delegate_;
	Module *owner;

	void invoke() {
	    delegate_(owner, this);
	}

    void setBoundedDelegate(const UnboundedDelegate &delegate_, Module *owner,
            BaseInputGate *inputGate = NULL) {
        ASSERT(delegate_.isSet());
        ASSERT(owner != NULL);
        this->owner = owner;
        this->delegate_ = delegate_;
        this->inputGate = inputGate;
    }

    template<typename... params>
    Callback<void(params...)> getCallback() {
        return TOLERANT_CALLBACK(&Message::invoke,*this, params...);
    }

private:
    Event event;
};

}

#endif /* MESSAGE_H_ */
