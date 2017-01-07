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

#define MODULE_NAME_LENGTH  5

/*INCLUDES-------------------------------------------------------------------*/
#include "Object.h"
#include "Message.h"
#include "Gate.h"
#include "types.h"

#include "Platform.h"

#include "TaskScheduler.h"


/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

extern TaskScheduler &getScheduler();

// declare Gate type
template<class C>
class Gate;

class Message;

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
 */
class Module: public Platform {
public:
	friend class Message;

	/**Allows accessing services at runtime using their name.
	 *
	 * @param name 	name of service
	 * @param idx   index of the module within an array of modules
	 * @return	pointer to service or NULL if service doesn't exist
	 */
	Module* getModule(const char* name, uint8_t idx = 0xff) const;

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
	virtual void initialize() {}

	const char* getName() const {
	    return name;
	}

	void setName(const char* );

	bool isScheduled(Message *msg);

	void cancel(Message *msg);

    void finish() {
        ASSERT(false); // should never be called, only for omnet compatibility
    }

	template<class M, class C>
	void schedule(M* msg, void(C::*method)(M*), timeOffset_t ms = 0) {
		msg->setBoundedDelegate(UnboundedDelegate(method), this);
		if (msg) {
			getScheduler().add(*msg, ms);
		}
	}

	template<class M, class C>
	void reschedule(M* msg, void(C::*method)(M*), timeOffset_t ms = 0) {
		msg->setBoundedDelegate(UnboundedDelegate(method), this);
		getScheduler().replace(*msg, ms);
	}

	template<class M, class C>
	TypedDelegate<M> createCallback(void(C::*method)(M *)) {
		return TypedDelegate<M> ((C*) this, method);
	}

	/**
	 * Calls initialize for all existing services.
	 */
	static void initializeAll();



#ifdef ENABLE_LOGGING
	uint8_t log_level;
	void setLogLevel(uint8_t level) {log_level=level;}
#else
	void setLogLevel(uint8_t level) {}
#endif


private:
	Module* next;
	static Module* first;
	static int numModules;

	/** Name of service
	 */
	char name[MODULE_NAME_LENGTH];

};

}

#endif /* MODULE_H_ */
