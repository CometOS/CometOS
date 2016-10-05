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

#ifndef GATE_H_
#define GATE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include "types.h"
#include "logging.h"
#include "Delegate.h"

#include "TaskScheduler.h"

namespace cometos {
	extern TaskScheduler &getScheduler();
}


/*MACROS---------------------------------------------------------------------*/

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class Module;

class BaseInputGate {
};

/***
 * Input gate for a message of type M.
 */
template<class M>
class InputGate: public BaseInputGate {

	// declare InputGateArray as friend, thus public constructor is avoided
	template<class C, uint8_t SIZE>
	friend class InputGateArray;

public:

	/**
	 * @param owner 	owner module of gate
	 * @param method	handler message, for messages received by this gate
	 * @param name 		parameter currently only used in OMNeT++ implementation
	 */
	template<class T>
	InputGate(T* owner, void(T::*method)(M*), const char* name) :
			delegate(method), owner(owner) {
	}
	void receive(M *msg, timeOffset_t ms = 0) {
		msg->setBoundedDelegate(delegate, owner, this);
		if (msg) {
		    getScheduler().add(*msg,ms);
		}
	}

private:
	InputGate() {
	}

	UnboundedDelegate delegate;
	Module *owner;

};

/**
 * Input gate array of length SIZE for given message type M.
 */

template<class M, uint8_t SIZE>
class InputGateArray {
public:

	template<class T>
	InputGateArray(T* owner, void(T::*method)(M*), const char* name) {
		for (uint8_t i = 0; i < SIZE; i++) {
			gates[i].delegate = UnboundedDelegate(method);
			gates[i].owner = owner;
		}
	}

	uint8_t getLength() {
		return SIZE;
	}

	InputGate<M>& get(uint8_t index) {
	    ASSERT(index < SIZE);
	    return gates[index];
	}

	uint8_t getIndex(M * msg) {
		BaseInputGate * gate = msg->inputGate;
		for (uint8_t i = 0; i < SIZE; i++) {
			if (&gates[i] == gate) {
				return i;
			}
		}
		ASSERT(false);
		return SIZE;
	}

private:
	InputGate<M> gates[SIZE];

	InputGateArray() {}

};

class BaseOutputGateArray;

/**
 * Output gate for a message of type M.
 */
template<class M>
class OutputGate {
	// declate OutputGateArray as friend, thus public constructor is avoided
	template<class C, uint8_t SIZE>
	friend class OutputGateArray;
public:

	OutputGate(Module *owner, const char* name) :
			targetGate(NULL) {
	}

	bool isConnected() {
	    return targetGate != NULL;
	}

	void send(M *msg, timeOffset_t ms = 0) {
		if (targetGate != NULL) {
		    targetGate->receive(msg, ms);
		} else {
		    ASSERT(false); 
		    // There are unconnected gates!
		    // Maybe you have built the wrong stack?
		    // (e.g. enabled use_default_stack, but do not use it)

		    // TODO the following can be disastrous with messages which are
		    //      allocated statically!!!!!!!
		    delete(msg);
		    return;
		}

	}

	void connectTo(InputGate<M> &targetGate) {
		this->targetGate = &targetGate;
	}

private:
	OutputGate() :
			targetGate(NULL) {
	}

	InputGate<M>* targetGate;
};

/**
 * Output gate array of length SIZE for given message type M.
 */

template<class M, uint8_t SIZE>
class OutputGateArray {
public:

	OutputGateArray(Module *owner, const char* name) {
	}

	OutputGate<M>& get(uint8_t index) {
		ASSERT(index < SIZE);
		return gates[index];
	}

	bool isConnected(uint8_t index) {
	    if (index < SIZE) {
	        return gates[index].isConnected();
	    } else {
	        return false;
	    }
	}

	void send(uint8_t index, M *msg, timeOffset_t ms = 0) {
		if (index < SIZE) {
		    gates[index].send(msg, ms);
		} else {
		    ASSERT(false);
		    // There are unconnected gates!
		    // Maybe you have built the wrong stack?
		    // (e.g. enabled use_default_stack, but do not use it)

		    // TODO the following can be disastrous with messages which are
		    //      allocated statically!!!!!!!
		    delete(msg);
		    return;
		}

	}

	uint8_t getLength() {
		return SIZE;
	}

private:
	OutputGate<M> gates[SIZE];

};

}

#endif /* GATE_H_ */
