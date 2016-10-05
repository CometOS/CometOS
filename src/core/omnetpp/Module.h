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

#ifndef MODULE_H_
#define MODULE_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Object.h"
#include "Message.h"
#include "Gate.h"

#include "types.h"
#include <omnetpp.h>

//#include "logging.h"

// OMNETPP
#include <list>
#include <map>

#include "Delegate.h"

#define MODULE_NAME_LENGTH 255

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

// declare Gate type
template<class C>
class Gate;


/**
 * This class provides basic functionality for writing modules/protocols in CometOS.
 * A major functionality is the scheduling facility for classes of types Message. T
 * Gates are used to connect modules in order to use message-passing approach as
 * inter-module communication (similar to the OMNeT++).
 *
 * All modules are named instances and stored in an internal list. This allows to access
 * modules by its name. For this purpose the method command(const char *argv[], const uint8_t argc)
 * can be overridden in order to process a string input.
 * Note that the initialize method of Module has to be called always (nevertheless it can be overridden).
 * In order to initialize all existing modules use the CometOS::initialize() (platform only).
 *
 * In OMNeT++ this class derives directly from cSimpleModule. It is not recommend to use methods
 * of the base class.
 */
class Module: public omnetpp::cSimpleModule {
public:

	/**Allows accessing services at runtime using their name.
	 *
	 * @param name 	name of service
	 * @param idx   index of the module within an array of modules
	 * @return	pointer to service or NULL if service doesn't exist
	 */
	Module* getModule(const char* name, uint8_t idx = 0xff);

	/**Constructor
	 *
	 * @param name	unique name of service (at most SERVICE_NAME_LENGTH characters), in case of OMNETPP
	 * 			the name is set via NED file
	 */
	Module(const char* service_name = NULL);

	/**
	 * 
	 */
	virtual ~Module();

	/**
	 * Can be implemented by derived classes in order to do some additional
	 * initialization. You should always call the base implementation of this
	 * function.
	 *
	 */
	virtual void initialize();

	/**
	 * Derived classes can optionally override this method in order to be controlled via
	 * text strings.
	 *
	 * @param argv	array of strings
	 * @param argc	length of argv
	 *
	 */
	virtual Object* command(const char *argv[], const uint8_t argc);

	/**
	 * @return identifier of node
	 */
	node_t getId() const;

	/**
	 * @return local time
	 */
	timestamp_t getTime();

	/**@return name of instance
	 */
	const char* getName() const;


	/**@return true if message is scheduled.
	 */
	bool isScheduled(Message *msg);

	/**Cancels scheduled self message. Note that this function doesn't call
	 * delete on message instance.
	 */
	void cancel(Message *msg);

	/**OMNETPP API*/

	/**
	 * Allows to postpone processing of a message by a given amount of time. After
	 * expiration of the timer the timeout message is called with the message
	 * instance.
	 *
	 * @param timer	valid pointer to allocated timer
	 * @param ms	time to postpone timer handling in milliseconds
	 *
	 */
	template<class Msg, class Mod>
	void schedule(Msg *msg, void(Mod::*handler)(Msg *), timeOffset_t ms = 0) {
		schedule_dbl(msg, handler, ms / 1000.0);
	}

	void check(Message *msg);

	template<class Msg, class Mod>
	void schedule_dbl(Message *msg, void(Mod::*handler)(Msg *), double s) {
		ASSERT(msg != NULL);
		ASSERT(handler != NULL);
		check(msg);
		InputGate<Msg>* p = new InputGate<Msg>(static_cast<Mod*>(this), handler,
				"timer", true);

		schedule_dbl(msg, p, s);

		//	sendDelayed(msg, s, timer.out);
	}

	void schedule_dbl(Message *msg, BaseInputGate* gate, double s);

	/**
	 * Like schedule, but timer is rescheduled, if already in queue
	 */
	template<class Msg, class Mod>
	void reschedule(Msg *msg, void(Mod::*handler)(Msg *), timeOffset_t ms = 0) {
		cancel(msg);
		schedule(msg, handler, ms);
	}

	template<class M, class C>
	TypedDelegate<M> createCallback(void(C::*method)(M *)) {
		return TypedDelegate<M>((C*)this,method);
	}

	template<class Msg, class Mod>
	InputGate<Msg> *createInputGate(void(Mod::*handler)(Msg *),
			const char * name, uint8_t index = 0xFF) {
		ASSERT(dynamic_cast<Mod*>(this) != NULL);
		// only if RTTI is available
		//inputGateDescriptor_t temp;
		//BoundedInputGate<Mod, Msg>* p = new BoundedInputGate<Mod, Msg> (
		//		static_cast<Mod*> (this), handler);
		/*	temp.gate = p;
		 temp.name = name;
		 temp.index = index;
		 inputGates.push_back(temp);
		 return p;*/
		return NULL;
	}

	template<class Msg>
	OutputGate<Msg> *createOutputGate(const char * name, uint8_t index = 0xFF) {
		//outputGateDescriptor_t temp;

		/*BoundedOutputGate<Module, Msg> *p = new BoundedOutputGate<Module, Msg> (
		 this);
		 temp.gate = p;
		 temp.name = name;
		 temp.index = index;
		 outputGates.push_back(temp);
		 return p;*/
		return NULL;
	}

public:

	std::list<BaseInoutGate*> inoutGates;

	std::list<BaseInputGate*> inputGates;
	std::list<BaseOutputGate*> outputGates;

	std::list<BaseInputGateArray*> inputGateArrays;
	std::list<BaseOutputGateArray*> outputGateArrays;

	/**OMNETPP API */
	std::map<omnetpp::cGate*, BaseInputGate *> gateConverter;

	/**OMNETPP API:  Since target module may create packet, a context switch is necessary
	 */
	Object* invoke_command(const char *argv[], const uint8_t argc) {
		Enter_Method_Silent();
		return command(argv, argc);
	}

	/**OMNETPP API */
	template<class T>
	T* getModule(const char* name) {
		return omnetpp::check_and_cast<T*>(
				getParentModule()->getModuleByPath(name));
	}

	/**
	 * OMNETPP API : Receive method for messanges of OMNET++ framework. Do not
	 * override this method!
	 */
	void handleMessage(omnetpp::cMessage *msg);

	/**OMNETPP API: This method is only implemented in Omnetpp. Try to use postpone.
	 */
	void schedule_dbl(Message *msg, double s);

	void initializeGates();

	/**OMNETPP API
	 * @inheritDoc
	 */
	void initialize(int stage) {
		if (stage == 1) {
			initializeGates();
			initialize();
		}
	}

	/**OMNETPP API
	 * @inheritDoc
	 */
	int numInitStages() const {
		return 2;
	}

	void scheduleAndPassOwnership(omnetpp::cPacket* m) {
		Enter_Method_Silent();
		omnetpp::cSimpleModule::take(m);
		scheduleAt(omnetpp::simTime(), m);
	}


	/**OMNETPP API */
//	node_t id;
};

}

#endif /* MODULE_H_ */
