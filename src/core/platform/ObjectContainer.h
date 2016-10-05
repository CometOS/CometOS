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

#ifndef OBJECTCONTAINER_H_
#define OBJECTCONTAINER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include "UniqueClassId.h"
#include "logging.h"
#include "Object.h"
#include "cometosAssert.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

/**
 * This container allows an aggregation of arbitrary object. Note that from each class only
 * one occurrence can be stored. The access is done via the class names.
 */
class ObjectContainer: public Object {
public:

	virtual Object* getCopy() {
		return new ObjectContainer(*this);
	}

	ObjectContainer& operator=(const ObjectContainer& obj) {
	    if (&obj != this) {
            removeAll();

            // get pointer to object list
            Object* p = obj.next_;

            // iterate
            while (p != NULL) {
                Object * n = p->getCopy();
                n->id_ = p->id_;
                n->next_ = next_;
                next_ = n;

                p = p->next_;
            }
	    }
		return *this;
	}

	ObjectContainer() {
	}

	/**Copy Constructor
	 */
	ObjectContainer(const ObjectContainer& p) {
		*this = p;
	}

	void removeAll() {
		Object* p = next_;
		while (p != NULL) {
			Object* r = p;
			p = p->next_;
			delete r;
		}
		next_ = NULL;
	}

	virtual ~ObjectContainer() {
		removeAll();
	}

	template<class T>
	T* getUnsafe() const {
		uint8_t id = UniqueClassId::get<T>();
		Object* p = next_;

		while (p != NULL) {
			if (p->id_ == id) {
				return (T*) p;
			}
			p = p->next_;
		}
		return NULL;
	}

	template<class T>
	T* get() const {
		T* p = getUnsafe<T> ();
		ASSERT(p!=NULL);
		return p;
	}

	template<class T>
	bool has() const {
		return NULL != getUnsafe<T> ();
	}

	template<class T>
	void set(T* pointer) {
	    if (pointer != NULL && pointer != nullptr) {
            remove<T> (); // remove old object of class if present
            pointer->id_ = UniqueClassId::get<T>();
            pointer->next_ = next_;
            next_ = pointer;
	    }
	}

	/**
	 * @return  pointer to instance of class, removes object from internal list, but doesn't
	 * 			invoke delete on object
	 * */
	template<class T>
	T* unset() {

		uint8_t id = UniqueClassId::get<T>();
		Object* p = next_;
		Object* prev = this;

		while (p != NULL) {
			if (p->id_ == id) {
				prev->next_ = p->next_;
				return (T*) p;
			}
			prev = p;
			p = p->next_;
		}
		return NULL;
	}

	template<class T>
	void remove() {
		T* p = unset<T> ();
		if (p)
			delete p;
	}
};

}

#endif /* OBJECTCONTAINER_H_ */
