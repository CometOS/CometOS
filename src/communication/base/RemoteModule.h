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

#ifndef REMOTEMODULE_H_
#define REMOTEMODULE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "Airframe.h"
#include "RemoteMethod.h"
#include "RemoteEvent.h"
#include "DList.h"
#include <string.h>

#define REMOTE_NAME_LENGTH		5

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

/**Wrapper for gerneric serialization of objects
 *
 */
class BaseSerializer {
public:
    virtual ~BaseSerializer() {};

	virtual void serialize(const void *instance, AirframePtr frame)=0;
	virtual void deserialize(void *instance, AirframePtr frame)=0;
};

/**Singleton class for Serialization of types
 */
template<class T>
class Serializer: public BaseSerializer {
public:
	static Serializer<T> &getInstance() {
		static Serializer<T> serializer;
		return serializer;
	}

	virtual ~Serializer() {}

	virtual void serialize(const void *instance, AirframePtr frame) {
		(*frame) << *((const T*) instance);
	}
	virtual void deserialize(void *instance, AirframePtr frame) {
		(*frame) >> *((T*) instance);

	}
private:
	Serializer() {
	}
};

class RemoteVariable {
public:
	RemoteVariable(void *variable, const char* name,
			BaseSerializer *serializer) {

		ASSERT(variable!=NULL);
		ASSERT(name!=NULL);
		ASSERT(serializer!=NULL);
		this->variable = variable;
		strncpy(this->name, name, REMOTE_NAME_LENGTH);
		this->serializer = serializer;

	}

	char name[REMOTE_NAME_LENGTH];
	void *variable;
	BaseSerializer *serializer;
};

/**Remote modules providing an interface for a direct access of
 * variables and methods. For this purpose a Transport-Layer for the
 * remote access has to be defined. Note that for accessing modul's
 * variables and method the module name is required. The declaration
 * of remote types is done during runtime!
 *
 * Typical applications for RemoteModule is the read out of monitored
 * data, logging, controlling, starting/stopping of services, etc.
 *
 */
class RemoteModule: public Module {
	friend class RemoteEventBase;
public:

	RemoteModule(const char *name) :
			Module(name), next(NULL), nextEvent(NULL) {
	}

	virtual ~RemoteModule() {
		for (DList<RemoteVariable*>::iterator it = remoteVariables.begin();
				it != remoteVariables.end(); it++) {
			delete (*it);
		}
		while (next != NULL) {
		    RemoteMethod* tmp = next->next;
		    delete next;
		    next = tmp;
		}
	}

	// subscribe remote event
	bool eventSubscribe(const char* name, void *ra, node_t dst, uint16_t counter) {
		RemoteEventBase *it = nextEvent;
		while (it) {
			if (strcmp(it->getName(), name) == 0) {
				return it->subscribe(ra, dst,counter);
			}
			it = it->next;
		}
		return false;
	}

	bool eventUnsubscribe(const char* name, node_t dst) {
		RemoteEventBase *it = nextEvent;
		while (it) {
			if (strcmp(it->getName(), name) == 0) {
				return it->unsubscribe(dst);
			}
			it = it->next;
		}
		return false;
	}

	/**Declares a variable as remotely acceessible.
	 *
	 * @param var	variable
	 * @param name	name which is used for remote access. Max length is REMOTE_NAME_LENGTH
	 * 				characters!
	 *
	 * Should be called in the initialize method.
	 */
	template<class T>
	void remoteDeclare(T& var, const char* name) {
		// check if name is already assigned
		ASSERT(remoteFindVariable(name)==NULL);
		remoteVariables.push_back(
				new RemoteVariable(&var, name, &Serializer<T>::getInstance()));
	}

	RemoteVariable *remoteFindVariable(const char* name) {
		for (DList<RemoteVariable*>::iterator it = remoteVariables.begin();
				it != remoteVariables.end(); it++) {
			if (strcmp((*it)->name, name) == 0) {
				return (*it);
			}
		}
		return NULL;
	}

	bool remoteWriteVariable(AirframePtr frame, const char* name) {
		RemoteVariable *var = remoteFindVariable(name);
		if (var == NULL) {
			return false;
		}

		return true;
	}

	bool remoteReadVariable(AirframePtr frame, const char* name) {
		RemoteVariable *var = remoteFindVariable(name);
		if (var == NULL) {
			return false;
		}
		var->serializer->serialize(var->variable, frame);
		return true;
	}

	RemoteMethod *remoteFindMethod(const char* name) {
		RemoteMethod *it = next;
		while (it) {
			if (strcmp(it->name, name) == 0) {
				return it;
			}
			it = it->next;
		}
		return NULL;
	}

	AirframePtr remoteMethodInvocation(AirframePtr frame, const char* name) {
#ifdef OMNETPP
		Enter_Method_Silent();
#endif
		RemoteMethod *method = remoteFindMethod(name);
		if (method == NULL) {
			frame.deleteObject();
		}

		return method->invoke(frame);
	}

	/**Interface for declaring synchronous or asynchronous remote methods.
	 * Method will act synchronous if no event name is given and will cause the
	 * event specified by the given name to be subscribed when the method is
	 * called. The module owning the method the can raise the event to signal
	 * asynchronous completion of the operation.
	 *
	 * @param method pointer to the method to be called
	 * @param name   name of the method to be used to identify the method;
	 *               MUST NOT be longer than AirString::MAX_LEN
	 * @param evName name of the event associated with this method (will cause
	 *               automatic subscription to the event if the method is called
	 *               remotely.
	 */
	template<class C>
	void remoteDeclare(void(C::*method)(), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethod<C>(name, evName, next, (C*) this, method);
	}

	template<class C, class Ret>
	void remoteDeclare(Ret(C::*method)(), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodRet<C, Ret>(name, evName, next, (C*) this, method);
	}

	template<class C, class Ret, class P1>
	void remoteDeclare(Ret(C::*method)(P1 &), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodRetP1<C, Ret, P1>(name, evName, next, (C*) this,
				method);
	}

	template<class C, class Ret, class P1, class P2>
	void remoteDeclare(Ret(C::*method)(P1 &, P2 &), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodRetP1P2<C, Ret, P1, P2>(name, evName, next,
				(C*) this, method);
	}

	template<class C, class Ret, class P1, class P2, class P3>
	void remoteDeclare(Ret(C::*method)(P1 &, P2 &, P3 &), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodRetP1P2P3<C, Ret, P1, P2, P3>(name, evName, next,
				(C*) this, method);
	}

	template<class C, class P1>
	void remoteDeclare(void(C::*method)(P1 &), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodP1<C, P1>(name, evName, next, (C*) this, method);
	}

	template<class C, class P1, class P2>
	void remoteDeclare(void(C::*method)(P1 &, P2 &), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodP1P2<C, P1, P2>(name, evName, next, (C*) this,
				method);
	}

	template<class C, class P1, class P2, class P3>
	void remoteDeclare(void(C::*method)(P1 &, P2 &, P3 &), const char* name, const char* evName = NULL) {
		ASSERT(remoteFindMethod(name)==NULL);
		next = new TypedRemoteMethodP1P2P3<C, P1, P2, P3>(name, evName, next, (C*) this,
				method);
	}


	// remoteDeclare for Methods
	//void remoteDeclare(T& var,const char* name);

	/**The remote event allows directly sending a variable
	 * to a base station. Note that calling this method has
	 * no effect unless the specific event is enabled.
	 * Enabling an event can be done via remoteSetEvent or
	 * is done by remote device (base station).
	 *
	 * Remote events can also be the base for a logging service.
	 */
	//template<class T>
	//void remoteRaiseEvent(T& var, const char* name);
	//void remoteRaiseEvent(const char* name);
	/**Sets or unsets remote event.
	 */
	//void remoteSetEvent(const char* name, bool set = true);
private:

	DList<RemoteVariable*> remoteVariables;
	RemoteMethod *next;
	RemoteEventBase *nextEvent;

};

} /* namespace cometos */
#endif /* REMOTEMODULE_H_ */
