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

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include "HashSet.h"

namespace cometos {

template<class K, class C, uint16_t LENGTH>
class HashMap: protected HashSet<K, LENGTH> {
public:
	C* get(K key) {
		uint8_t index;
		if (HashSet<K, LENGTH>::contains(key, index)) {
			return &elements[index];
		}
		return NULL;
	}

	bool set(K key, C& value) {
		uint8_t index;
		if (HashSet<K, LENGTH>::contains(key, index)) {
			elements[index] = value;
			return true;
		}
		if (HashSet<K, LENGTH>::size == LENGTH) {
			return false;
		}
		HashSet<K, LENGTH>::values[index] = key;
		elements[index] = value;
		HashSet<K, LENGTH>::present.set(index, true);
		HashSet<K, LENGTH>::size++;
		return true;
	}

	bool keysToList(StaticSList<K, LENGTH> & list) {
	    return this->valuesToList(list);
	}

	void remove(K key) {
		HashSet<K, LENGTH>::remove(key);
	}

	void clear() {
		HashSet<K, LENGTH>::clear();
	}

	inline bool empty() {
		return HashSet<K, LENGTH>::empty();
	}

	inline bool full() {
		return HashSet<K, LENGTH>::getSize() == LENGTH;
	}

	inline uint8_t getSize() {
		return HashSet<K, LENGTH>::getSize();
	}

	inline uint16_t getMaxSize() {
		return LENGTH;
	}

	bool contains(K val) {
		return HashSet<K, LENGTH>::contains(val);
	}

protected:
	C elements[LENGTH];
};


}

#endif /* HASHMAP_H_ */
