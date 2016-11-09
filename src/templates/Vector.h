#ifndef VECTOR_H_
#define VECTOR_H_

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

/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include "cometosAssert.h"

namespace cometos {

/**
 * Vectors are a kind of sequence container. As such,
 * their elements are ordered following a strict linear sequence.
 * This vector class adopts parts of the interface provided
 * by the STL class Vector. However, no dynamic C array is used.
 * Instead, a C array of an appropriate size must be given by the user.
 *
 * Note that the use of this class with instances with non-empty
 * destructor is not recommend, since a clear/pop will not call this.
 * It is recommend to use this class conjunction with pointers or
 * "plain structs/classes"
 *
 * Random access  is currently only supported for reading in order to
 * force correct usage in CometOS framework (TODO, change this).
 */
template<class C>
class BaseVector {
public:

	BaseVector(C* pBuffer, uint8_t max_size) :
			pBuffer(pBuffer), size(0), max_size(max_size) {
	}

	/**Clears array.*/
	void clear() {
		size = 0;
	}

	uint8_t count(const C & val) const {
		uint8_t c = 0;
		for (uint8_t i = 0; i < getSize(); i++) {
			if (pBuffer[i] == val) {
				c++;
			}
		}
		return c;
	}

	/**Returns current size of array
	 */
	inline uint8_t getSize() const {
		return size;
	}

	/**Returns maximum possible size
	 */
	inline uint8_t getMaxSize() const {
		return max_size;
	}

	inline C* getBuffer() {
		return pBuffer;
	}

	inline const C* getConstBuffer() const {
		return pBuffer;
	}

	/**Appends element to the end of the array
	 */
	void pushBack(const C& x) {
		ASSERT(size < max_size);
		pBuffer[size] = x;
		size++;
	}

	/**Sets new size of Vector. Note that no data is initialized.*/
	void setSize(uint8_t size) {
		ASSERT(size <= max_size);
		this->size = size;

	}

	/**Removes last element,
	 * and return copy of this
	 */
	const C& popBack() {
		ASSERT(size > 0);
		size--;
		return pBuffer[size];
	}

	C& get(uint8_t index) {
		return this->operator[](index);
	}

	C& operator[](uint8_t index) {
	    if (index >= size) {
	        ASSERT(false);
	    }
	    return pBuffer[index];
	}

	const C& operator[](uint8_t index) const {
		ASSERT(index < size);
		return pBuffer[index];
	}

	BaseVector<C> &operator=(const BaseVector<C> & x) {
		this->clear();
		for (uint8_t i = 0; i < x.getSize(); i++) {
			this->pushBack(x[i]);
		}
		return *this;
	}


	/**Prepends element to the front of the array, O(N)!
	 */
	void pushFront(const C& x) {
		ASSERT(size < max_size);
        for(uint8_t to = size; to > 0; to--) {
            pBuffer[to] = pBuffer[to-1];
        }
		pBuffer[0] = x;
		size++;
	}
    
	/**Removes first element, and return copy of this, O(N)!
	 */
	C popFront() {
		ASSERT(size > 0);
        C element = pBuffer[0];
        for(uint8_t to = 0; to < size-1; to++) {
            pBuffer[to] = pBuffer[to+1];
        }
		size--;
		return element;
	}

private:
	C* pBuffer;
	uint8_t size;
	const uint8_t max_size;

};

template<class C, uint8_t MAX_SIZE>
class Vector: public BaseVector<C> {
public:
	Vector() :
			BaseVector<C>(buffer, MAX_SIZE) {
	}

	Vector(const Vector<C, MAX_SIZE> &vector) :
		BaseVector<C>(buffer, MAX_SIZE) {
		this->operator=(vector);
	}

private:
	C buffer[MAX_SIZE];
};

//class ByteVector: public BaseVector<uint8_t> {
//public:
//    ByteVector(uint8_t* pBuffer, uint8_t max_size) :
//        BaseVector(pBuffer, max_size) {}
//
//    ByteVector &operator=(const ByteVector & x) {
//        memcpy(getBuffer(), x.getConstBuffer(), x.getSize());
//        setSize(x.getSize());
//        return *this;
//    }
//};


typedef BaseVector<uint8_t> ByteVector;

}


#endif /* VECTOR_H_ */
