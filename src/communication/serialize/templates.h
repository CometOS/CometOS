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

#ifndef TEMPLATES_H_
#define TEMPLATES_H_

// contains serialization function for template types
// TODO Add iterator classes for template containers
#include "Vector.h"
#include "SList.h"
#include "Tuple.h"

namespace cometos {

template<class C>
void serialize(ByteVector& buffer, const BaseVector<C>& value) {

	uint8_t size = value.getSize();
	for (uint8_t i = size; i > 0; i--) {
		serialize(buffer, value[i - 1]);
	}
	serialize(buffer, size);
}

template<class C>
void unserialize(ByteVector& buffer, BaseVector<C>& value) {
	value.clear();
	uint8_t size;
	C item;
	unserialize(buffer, size);
	for (uint8_t i = 0; i < size; i++) {
		unserialize(buffer, item);
		value.pushBack(item);
	}
}




} /* namespace cometos */


template<class T1>
void serialize(cometos::ByteVector& buffer, const Tuple<T1>& value) {
    serialize(buffer,value.first);
}

template<class T1>
void unserialize(cometos::ByteVector& buffer, Tuple<T1>& value) {
    unserialize(buffer,value.first);
}

template<class T1,class T2>
void serialize(cometos::ByteVector& buffer, const Tuple<T1,T2>& value) {
	serialize(buffer,value.first);
	serialize(buffer,value.second);
}

template<class T1,class T2>
void unserialize(cometos::ByteVector& buffer, Tuple<T1,T2>& value) {
	unserialize(buffer,value.second);
	unserialize(buffer,value.first);
}

template<class T1,class T2,class T3>
void serialize(cometos::ByteVector& buffer, const Tuple<T1,T2,T3>& value) {
	serialize(buffer,value.first);
	serialize(buffer,value.second);
	serialize(buffer,value.third);
}

template<class T1,class T2, class T3>
void unserialize(cometos::ByteVector& buffer, Tuple<T1,T2,T3>& value) {
	unserialize(buffer,value.third);
	unserialize(buffer,value.second);
	unserialize(buffer,value.first);
}


#endif /* TEMPLATES_H_ */
