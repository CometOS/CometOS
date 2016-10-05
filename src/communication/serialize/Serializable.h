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

#ifndef SERIALIZABLE_H_
#define SERIALIZABLE_H_

#ifndef NO_SERIALIZABLE_WRAPPER

#include <stdint.h>
#include "logging.h"
#include "Vector.h"
#include "primitives.h"
#include "templates.h"
#include "OutputStream.h"

#ifndef INT8_MIN
#define INT8_MIN -128
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif


/**Maximum size of data array on
 * which this class works is currently 128 Byte
 *
 * List is always closed (has no start or end)
 */
class LinkedOffsetList {

public:
	LinkedOffsetList();

	LinkedOffsetList * getNext();

	LinkedOffsetList * getPrev();

	void insertAfter(LinkedOffsetList* item);

private:

	LinkedOffsetList(const LinkedOffsetList& copy) {}


	static void link(LinkedOffsetList* first, LinkedOffsetList* second);

	static bool checkOffset(intptr_t offset);

	int8_t next;
	int8_t prev;
};

class FieldBase: public LinkedOffsetList {
public:
	FieldBase(FieldBase* prev_);

	virtual ~FieldBase() {}

	virtual void serialize_(cometos::ByteVector& buffer) {
	}
	virtual void deserialize_(cometos::ByteVector& buffer) {
	}
};

template<class T>
class Field: public FieldBase {
public:

	Field(FieldBase* prev_) :
			FieldBase(prev_) {
	}

	Field(const T &defData, FieldBase* prev_) :
			FieldBase(prev_), data(defData) {
	}

	virtual ~Field() {}

	virtual void serialize_(cometos::ByteVector& buffer) {
		serialize(buffer,data);
	}

	virtual void deserialize_(cometos::ByteVector& buffer) {
		unserialize(buffer,data);
	}

	T *operator->() {
		return &data;
	}


	Field<T>& operator = (const T& data) {
		this->data=data;
		return *this;
	}

	operator T ()  {
		return data;
	}

	T operator()() const {
	    return data;
	}
private:
	T data;
};


/**A convenient way to provide serializable classes.
 * Example:
 *
 * <code>
 * class MyHeader : public Serializable {
 *     public:
 *     MyHeader() : a(0xff, this), b(this) {}
 *     Field<uint8_t> a;
 *     Field<AirString> b;
 * };
 *
 * MyHeader header;
 *
 * header.a=3;
 * if (5>header.a) {
 *     cout<<header.b->getStr();
 * }
 *
 * <\code>
 */
class Serializable: public FieldBase {
public:
	Serializable();

	Serializable(FieldBase* prev_);

	virtual ~Serializable() {
	}

	virtual void serialize_(cometos::ByteVector& buffer);

	virtual void deserialize_(cometos::ByteVector& buffer);
};

void serialize(cometos::ByteVector& buffer,  const Serializable& value);

void unserialize(cometos::ByteVector& buffer, Serializable& value);

template<class T>
cometos::OutputStream& operator<<(cometos::OutputStream& lhs, const Field<T>& rhs) {
    return lhs << rhs();
}



#endif

#endif /* SERIALIZABLE_H_ */
