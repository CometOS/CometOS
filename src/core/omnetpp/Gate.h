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

#ifndef GATE_H_
#define GATE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <omnetpp.h>
#include <iostream>
#include "types.h"



/*MACROS---------------------------------------------------------------------*/

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class Message;

/**Abstraction for callback/handlers with one argument
 */
template<class M>
class BaseHandler {
public:
	virtual ~BaseHandler() {
	}

	/**Handler method.
	 */
	virtual void invoke(M *msg)=0;
};

/**
 * Abstraction of C++ method pointers.
 */
template<class C, class M>
class Handler: public BaseHandler<M> {
public:

	/**
	 * @param owner	valid pointer to class
	 * @param handler	valid pointer to class method
	 */
	Handler(C* owner, void(C::*handler)(M *)) : owner(owner), handler(handler) {
		//this->owner = owner;
		//this->handler = handler;
	}

	virtual ~Handler() {}  // TODO clean up by deleting owner?

	virtual void invoke(M *msg) {
		omnetpp::cSimulation* sim = omnetpp::cSimulation::getActiveSimulation();
		ASSERT(sim!=NULL);
		omnetpp::cSimpleModule* module = sim->getContextSimpleModule();
		ASSERT(module!=NULL);
		if (owner != module) {
			owner->scheduleAndPassOwnership(msg);
		} else {
			(owner->*handler)(static_cast<M*>(msg));
		}
	}

private:
	/**
	 * Owner of this gate. Pointer must be valid
	 */
	C* owner;

	/**
	 * Callback method for this gate
	 */
	void (C::*handler)(M*);
};

/**
 * Base class for input gates.
 */
class BaseInputGate {
public:

	virtual ~BaseInputGate() {
	}

	/**
	 * Incoming messages are passed to this method.
	 * @param msg	valid message instance
	 */
	virtual void receiveMsg(Message *msg) {
		ASSERT(false);
	}

	/**Name of gate. This is used by OMNET++ to map the gate to the
	 * one declared in the NED file.
	 * */
	const char* name;

	/**This flag is used to identify anonymous/temporary gates.
	 * Such gates are deleted after they processes a message.
	 */
	bool anonymous;

protected:
	BaseInputGate() :
			name(NULL), anonymous(false) {
	}

};

class BaseInoutGate: public BaseInputGate {
public:

	BaseInoutGate() : BaseInputGate(), out(NULL) {}
	omnetpp::cGate * out;
};

template<class M>
class InoutGate: public BaseInoutGate {
	friend class BaseInputGateArray;
public:

	virtual ~InoutGate() {
		if (this->handler) {
			delete this->handler;
		}
	}

	/**
	 * @param owner		valid class
	 * @param handler	valid method pointer
	 * @param name		name of gate (used in OMNET++)
	 * @paran anonymous, if true this gate was created with new and will be deleted after receiving a message
	 */
	template<class C>
	InoutGate(C* owner, void(C::*handler)(M *), const char* name,
			bool anonymous = false) : BaseInoutGate(), handler(NULL) {
		this->anonymous = anonymous;
		this->name = name;
		this->handler = new Handler<C, M>(owner, handler);
		if (!anonymous) {
			owner->inoutGates.push_back(this);
		}
	}

	virtual void receiveMsg(Message *msg) {
		handler->invoke((omnetpp::check_and_cast<M*> )(msg));
	}

	BaseHandler<M>* handler;

	void send(Message* msg, timeOffset_t ms = 0) {
		ASSERT(out!=NULL);
		(omnetpp::check_and_cast<omnetpp::cSimpleModule*>(out->getOwnerModule()))->sendDelayed(
				(omnetpp::cMessage*) msg, ms / 1000.0, out);
	}

protected:

};

class BaseInputGateArray;

/***
 * Input gate for a message of type M.
 */
template<class M>
class InputGate: public BaseInputGate {
	friend class BaseInputGateArray;
public:

	virtual ~InputGate() {
		if (this->handler) {
			delete this->handler;
		}
	}

	/**
	 * @param owner		valid class
	 * @param handler	valid method pointer
	 * @param name		name of gate (used in OMNET++)
	 * @paran anonymous, if true this gate was created with new and will be deleted after receiving a message
	 */
	template<class C>
	InputGate(C* owner, void(C::*handler)(M *), const char* name,
			bool anonymous = false) : BaseInputGate(), handler(NULL) {
		this->anonymous = anonymous;
		this->name = name;
		this->handler = new Handler<C, M>(owner, handler);
		if (!anonymous) {
			owner->inputGates.push_back(this);
		}
	}

	virtual void receiveMsg(Message *msg) {
		handler->invoke((omnetpp::check_and_cast<M*> )(msg));
	}

	BaseHandler<M>* handler;

	InputGate() : BaseInputGate(), handler(NULL) {
	}

protected:

};

/**
 * Abstraction for input gate arrays.
 */
class BaseInputGateArray {
public:
    virtual ~BaseInputGateArray() {};
	virtual uint8_t getLength() =0;

	virtual BaseInputGate* getBaseGate(uint8_t index)=0;

};

/**
 * Input gate array of length SIZE for given message type M.
 */
template<class M, uint8_t SIZE>
class InputGateArray: public BaseInputGateArray {
public:

	template<class C>
	InputGateArray(C* owner, void(C::*handler)(M *), const char* name) {

		for (uint8_t i = 0; i < SIZE; i++) {
			gates[i].name = name;
			gates[i].handler = new Handler<C, M>(owner, handler);
		}
		owner->inputGateArrays.push_back(this);
	}

	virtual ~InputGateArray() {}

	uint8_t getLength() {
		return SIZE;
	}

	BaseInputGate* getBaseGate(uint8_t index) {
		ASSERT(index<SIZE);
		return &gates[index];
	}

	InputGate<M>& get(uint8_t index) {
		ASSERT(index<SIZE);
		return gates[index];
	}

	uint8_t getIndex(M * msg) {
		BaseInputGate * gate = msg->inputGate;
		for (uint8_t i = 0; i < SIZE; i++) {
			if (&gates[i] == gate) {
				return i;
			}
		}ASSERT(false);
		return SIZE;
	}

private:
	InputGate<M> gates[SIZE];

};

class BaseOutputGateArray;

/**
 * Base class for output gates.
 */
class BaseOutputGate {
	friend class BaseOutputGateArray;
public:
	template<class C>
	BaseOutputGate(C* owner, const char *name) :
			out(NULL), name(name) {
		owner->outputGates.push_back(this);
	}

	bool isConnected() {
	    return out != NULL;
	}

	omnetpp::cGate * out;
	const char* name;

protected:
	BaseOutputGate() :
			out(NULL), name(NULL) {

	}

	void send(Message* msg, timeOffset_t ms = 0) {
		ASSERT(out!=NULL);
		(omnetpp::check_and_cast<omnetpp::cSimpleModule*>(out->getOwnerModule()))->sendDelayed(
				(omnetpp::cMessage*) msg, ms / 1000.0, out);
	}

};

/**
 * Output gate for a message of type M.
 */
template<class M>
class OutputGate: public BaseOutputGate {
	friend class BaseOutputGateArray;
public:

	template<class C>
	OutputGate(C* owner, const char *name) :
			BaseOutputGate(owner, name) {
	}

	void send(M *msg, timeOffset_t ms = 0) {
		BaseOutputGate::send(msg, ms);
	}

	OutputGate() :
			BaseOutputGate() {
	}

    void connectTo(InputGate<M> &targetGate) {}

protected:
};

/**
 * Abstraction for output gate arrays.
 */
class BaseOutputGateArray {
public:
    virtual ~BaseOutputGateArray() {}
	virtual uint8_t getLength() =0;
	virtual BaseOutputGate* getBaseGate(uint8_t index)=0;

};

/**
 * Output gate array of length SIZE for given message type M.
 */
template<class M, uint8_t SIZE>
class OutputGateArray: public BaseOutputGateArray {
public:

	template<class C>
	OutputGateArray(C* owner, const char* name) {
		for (uint8_t i = 0; i < SIZE; i++) {
			gates[i].name = name;
		}
		owner->outputGateArrays.push_back(this);
	}

	virtual ~OutputGateArray() {}

	OutputGate<M>& get(uint8_t index) {
		ASSERT(index<SIZE);
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
		ASSERT(index<SIZE);
		gates[index].send(msg, ms);
	}

	uint8_t getLength() {
		return SIZE;
	}
	BaseOutputGate* getBaseGate(uint8_t index) {
		ASSERT(index<SIZE);
		return &gates[index];
	}

private:
	OutputGate<M> gates[SIZE];

};

}

#endif /* GATE_H_ */
