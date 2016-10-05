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

#ifndef DELEGATE_H_
#define DELEGATE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cometosAssert.h"

namespace cometos {
class Module;
}

/**Delegate implementation for methods with one pointer argument.
 * The stored method pointer is unbounded!
 *
 * inspired by JaeWook Choi's proposal
 * http://www.codeproject.com/KB/cpp/fd.aspx?display=Print
 *
 */
class UnboundedDelegate {
public:

	// just for retrieving size of method pointers at compile time
	typedef void (UnboundedDelegate::*MethodSize)(void*);

	template<class T, class M>
	UnboundedDelegate(void(T::*method)(M*)) {
		//TODO is this safe enough?
		if (method != (void(T::*)(M*))NULL) {
			set(method);
		}
	}

	bool isSet() const {
		return stub_ptr != 0;
	}

	UnboundedDelegate() :
			stub_ptr(0) {
	}
	template<class T, class M>
	struct fp_helper {
		inline static void init(UnboundedDelegate & dg, void(T::*method)(M*)) {
			typedef void (T::*TMethod)(M*);
			ASSERT(sizeof(MethodSize) == sizeof(TMethod));
			memcpy(dg.buf_, &method, sizeof(TMethod));
		}

		inline static void invoke(UnboundedDelegate const & dg, T* object_ptr,
				M* a1) {
			typedef void (T::*TMethod)(M*);
			TMethod const method = *reinterpret_cast<TMethod const *>(dg.buf_);
			(object_ptr->*method)(a1);
		}
	};

	template<class T, class M>
	void set(void(T::*method)(M*)) {
		fp_helper<T, M>::init(*this, method);
		stub_ptr = &Delegate_stub_t<T, M>::method_stub;
	}

	void unset() {
		stub_ptr = 0;
	}

	void operator()(cometos::Module *object_ptr, void* a1) const {
		ASSERT(stub_ptr != NULL);
		(*stub_ptr)(*this, object_ptr, a1);
	}
private:
	typedef void (*stub_type)(UnboundedDelegate const & dg, cometos::Module* object_ptr,
			void*);
	stub_type stub_ptr;
	unsigned char buf_[sizeof(MethodSize)];
	template<class T, class M>
	struct Delegate_stub_t {
		inline static void method_stub(UnboundedDelegate const & dg,
				cometos::Module* object_ptr, void* a1) {
			T* p = (T*) (object_ptr);
			fp_helper<T, M>::invoke(dg, p, (M*) (a1));
		}

	};
};

template<class M>
class TypedDelegate: private UnboundedDelegate {
public:

	TypedDelegate() :
			owner(NULL) {
	}

	template<class OWNER>
	TypedDelegate(OWNER* owner, void(OWNER::*method)(M*)) :
			owner(owner) {
		if (method != (void(OWNER::*)(M*))NULL) {
			set(method);
		}
	}

	bool isReady() {
		return UnboundedDelegate::isSet() && owner != NULL;
	}

	void operator()(M *m) const {
		ASSERT(owner != NULL);
		UnboundedDelegate::operator()(owner, m);
	}

    void setOwner(cometos::Module* newOwner) {
        owner = newOwner;
    }

private:
	cometos::Module *owner;
};

#endif /* DELEGATE_H_ */
