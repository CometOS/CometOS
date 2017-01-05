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

#ifndef MESSAGE_H_
#define MESSAGE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include "ObjectContainer.h"
#include "List.h"
#include "Gate.h"
#include "types.h"
#include "List.h"
#include "Delegate.h"
#include "Module.h"
#include "OutputStream.h"
#include "Event.h"

/*MACROS---------------------------------------------------------------------*/

#define MESSAGE_INVALID_GATE	0xff
#define MESSAGE_SIZE_LOG		20



#include "Task.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class BaseInputGate;



/**
 * Instances of this class are used for exchanging information among
 * services. It derives from UniqueObjectContainer in order to append
 * arbitrary data to this class.
 */
class Message: public Task, public ObjectContainer {

public:
	Message();

	void invoke() {
	   // getCout() << ((Module *) owner)->getName() << hex << (unsigned int) this << dec << endl;
	    delegate_(owner, this);
	}

	Event* getEvent() {
	    return &event;
	}


#ifdef ENABLE_LOGGING
	/**In case of enabled logging a task may define his log level,
	 * this log level can be used to filter log information during execution.*/
	virtual uint8_t getLogLevel();
#endif

	virtual const Module* getContext() const;

	Message& operator=(const Message &msg) {
		ObjectContainer::operator=(const_cast<Message &>(msg));
		return *this;
	}

	virtual ~Message();

	void setBoundedDelegate(const UnboundedDelegate &delegate_, Module *owner,
			BaseInputGate *inputGate = NULL) {
		ASSERT(delegate_.isSet());
		ASSERT(owner != NULL);
		ASSERT(offset == 0xFFFF);
		this->owner = owner;
		this->delegate_ = delegate_;
		this->inputGate = inputGate;
	}

    void removeBoundedDelegate() {
		this->owner = nullptr;
		this->delegate_.unset();
    }

	UnboundedDelegate &getDelegate() {
		return delegate_;
	}
	Module *getOwner() {
		return owner;
	}

private:
	UnboundedDelegate delegate_;

	/** Every owner has to be a base class of cometos::Module. This used
     * to be a pointer to void in the past. However, this is NOT SAFE in
     * presence of multiple inheritance, when the most derived class was the
     * actual type T for the method but a pointer to Module was
     * passed to the schedule method (@see Module::schedule). Explanation:
     *
     * Base1  Module            +------------------+ <-- DerivedModule, Base1
     *   |     |                | vptr Base1       |
     *   |     |                +------------------+ <-- Module
     *    \   /                 | vptr Module      |
     *     \ /                  | ModuleMember1    |
     * DerivedModule            |     ...          |
     *                          | ModuleMembern    |
     *                          |     ...          |
     *                          +------------------+
     *
     * With an inheritance hierarchy as shown on the left, the actual
     * arrangement of a DerivedModule object in memory looks something like
     * the picture on the right. Module::schedule now passes the this pointer
     * of Module, containing the offset to the Delegate constructor. This
     * pointer is then stored and later casted back to DerivedModule type
     * by means of the template parameters (instantiation in
     * UnboundedDelegate::set()).
     * (@see UnboundedDelegate::Delegate_stub_t::method_stub). In case we
     * downcast a Module pointer to a DerivedModule pointer, the compiler will
     * know about the offset it has to apply to the actual address and
     * everything is fine. However, if the Module pointer is converted to void*
     * in the meantime, there are no means to know about the necessary offset
     * and the cast will pointer will wrongly point to a wrong address.
     * This results in a wrong this pointer when the actual method call is
     * executed and is a sure recipe for disaster.
     */
	Module *owner;

public:
	timeOffset_t offset;

	Event event;

	// TODO should be removed in later release, delegate_ object should be enough
	BaseInputGate *inputGate;

#ifdef DEBUG_MESSAGE_ALLOCATION
	typedef List<Message*, MESSAGE_SIZE_LOG> MessageList;
	static MessageList& messageList();
	static void printOwners();
	static void printAllocationCount();
#endif
};

}

#endif /* MESSAGE_H_ */
