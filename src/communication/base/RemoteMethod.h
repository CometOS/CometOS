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

#ifndef REMOTEMETHOD_H_
#define REMOTEMETHOD_H_

#include "Airframe.h"
#include "RemoteEvent.h"

namespace cometos {

// TODO need interface for asynchronous operation

/**Implementation of synchronize method invocation*/
class RemoteMethod {
public:

	virtual ~RemoteMethod() {
	}

	RemoteMethod(const char *name, const char * evName, RemoteMethod* next) :
			name(name), next(next), evName(evName) {
	}

	virtual AirframePtr invoke(AirframePtr frame)=0;
	const char* name;
	RemoteMethod* next; // used for building linked list
	const char* evName;
};

template<class C>
class TypedRemoteMethod: public RemoteMethod {
public:

	typedef void (C::*METHOD)();

	virtual ~TypedRemoteMethod() {}

	TypedRemoteMethod(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName, next), owner(owner), method(method) {
	}

	virtual AirframePtr invoke(AirframePtr frame) {
		ASSERT(frame->getLength()==0);
		(owner->*method)();
		(*frame) << (uint8_t)(0);
		return frame;
	}

	C* owner;
	METHOD method;
};


template<class C, class Ret>
class TypedRemoteMethodRet: public RemoteMethod {
public:

	typedef Ret(C::*METHOD)(void);

	TypedRemoteMethodRet(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		ASSERT(frame->getLength()==0);
		(*frame) << ((owner->*method)());
		return frame;
	}

	C* owner;
	METHOD method;
};


template<class C, class Ret, class P1>
class TypedRemoteMethodRetP1: public RemoteMethod {
public:

	typedef Ret(C::*METHOD)(P1 &);

	TypedRemoteMethodRetP1(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		P1 val1;
		(*frame) >> val1;
		ASSERT(frame->getLength()==0);
		(*frame) << ((owner->*method)(val1));
		return frame;
	}

	C* owner;
	METHOD method;
};

template<class C, class Ret, class P1, class P2>
class TypedRemoteMethodRetP1P2: public RemoteMethod {
public:
	typedef Ret(C::*METHOD)(P1 &, P2 &);

	TypedRemoteMethodRetP1P2(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		P1 val1;
		P2 val2;
		(*frame) >> val2 >> val1;
		ASSERT(frame->getLength()==0);
		(*frame) << ((owner->*method)(val1, val2));
		return frame;
	}

	C* owner;
	METHOD method;
};

template<class C, class Ret, class P1, class P2, class P3>
class TypedRemoteMethodRetP1P2P3: public RemoteMethod {
public:
	typedef Ret(C::*METHOD)(P1 &, P2 &, P3 &);

	TypedRemoteMethodRetP1P2P3(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		P1 val1;
		P2 val2;
		P3 val3;
		(*frame) >> val3 >> val2 >> val1;
		ASSERT(frame->getLength()==0);
		(*frame) << ((owner->*method)(val1, val2, val3));
		return frame;
	}

	C* owner;
	METHOD method;
};



template<class C, class P1>
class TypedRemoteMethodP1: public RemoteMethod {
public:
	typedef void (C::*METHOD)(P1 &);

	TypedRemoteMethodP1(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		P1 val1;
		(*frame) >> val1;
		ASSERT(frame->getLength()==0);
		(owner->*method)(val1);
		(*frame) << (uint8_t) (0);
		return frame;
	}

	C* owner;
	METHOD method;

};

template<class C, class P1, class P2>
class TypedRemoteMethodP1P2: public RemoteMethod {
public:

	typedef void (C::*METHOD)(P1 &, P2 &);

	TypedRemoteMethodP1P2(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		P1 val1;
		P2 val2;
		(*frame) >> val2 >> val1;
		ASSERT(frame->getLength()==0);
		(owner->*method)(val1, val2);
		(*frame) << (uint8_t) (0);
		return frame;
	}

	C* owner;
	METHOD method;
};

template<class C, class P1, class P2, class P3>
class TypedRemoteMethodP1P2P3: public RemoteMethod {
public:

	typedef void (C::*METHOD)(P1 &, P2 &, P3 &);

	TypedRemoteMethodP1P2P3(const char *name, const char * evName, RemoteMethod* next, C* owner,
			METHOD method) :
			RemoteMethod(name, evName,  next), owner(owner), method(method) {
	}

	AirframePtr invoke(AirframePtr frame) {
		P1 val1;
		P2 val2;
		P3 val3;
		(*frame) >> val3 >> val2 >> val1;
		ASSERT(frame->getLength()==0);
		(owner->*method)(val1, val2, val3);
		(*frame) << (uint8_t) (0);
		return frame;
	}

	C* owner;
	METHOD method;
};

}

#endif /* REMOTE_H_ */
