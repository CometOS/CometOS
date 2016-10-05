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

#ifndef HASHSET_H_
#define HASHSET_H_

#include "BitVector.h"
#include "SList.h"

namespace cometos {

/**
 * HashSet uses %-operator for calculating the has value.
 * If value is not instantly found via hash function, set
 * is very slow.
 *
 * UnitTest exists in gcc tests
 * TODO Optimize
 *
 * */
template<class C, uint16_t LENGTH>
class HashSet {
public:

	HashSet() :
		size(0) {

	}

	/**Adds the specified element to this set if it is not already present (optional operation).
	 * @return false if container is full
	 */
	bool add(C val) {
		uint8_t index;

		// check if already present
		if (contains(val, index)) {
			return true;
		}

		if (size == LENGTH) {
			return false;
		}

		present.set(index, true);
		values[index] = val;
		size++;
		return true;
	}

	bool contains(C val) {
		uint8_t index;
		return contains(val, index);
	}

	void remove(C val) {
		uint8_t index;
		if (contains(val, index)) {
			size--;
			present.set(index, false);
		}
	}

	/**
	 * Appends all elements of this set to the provided SList
	 *
	 * @param list reference to an empty SList object which has enough space
	 *             to hold all elements of this HashSet
	 * @return true, if all elements where appended to the SList
	 *         false, if not (e.g. list not empty,
	 */
	bool valuesToList(StaticSList<C, LENGTH> & list) {
	    if (!list.empty()) {
	        return false;
	    } else {
	        for (uint16_t i=0; i<LENGTH; i++) {
	            if (present.get(i)) {
	                list.push_back(values[i]);
	            }
	        }
	        return true;
	    }
	}

	/**Removes all elements from set. Note that no destructor is called!*/
	void clear() {
		size = 0;
		present.fill(false);
	}

	inline bool empty() {
		return size == 0;
	}

	/**
	 * @return current number of elements in set
	 */
	inline uint8_t getSize() {
		return size;
	}

protected:

	bool contains(C &val, uint8_t &index) {
		index = getHash(val);

		for (uint8_t i = 0; (i < LENGTH); i++) {
			if (present.get(index) && values[index] == val)
				return true;
			index = (index + 1) % LENGTH;
		}

		// get next free position
		for (uint8_t i = 0; (i < LENGTH); i++) {
			if (!present.get(index))
				return false;
			index = (index + 1) % LENGTH;
		}

		return false;
	}

	uint8_t getHash(C &val) {
		return val % LENGTH;
	}

	C values[LENGTH];
	cometos::BitVector<LENGTH> present;
	uint8_t size;
};

}

#endif /* HASHSET_H_ */
