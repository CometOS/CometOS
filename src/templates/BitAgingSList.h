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
 * @author Stefan Unterschuetz
 */

#ifndef BITAGINGSLIST_H_
#define BITAGINGSLIST_H_

#include <stdint.h>
#include "SList.h"
#include "BitVector.h"

namespace cometos {

/**Implementation of BitAging algorithm for SList container. This class replace DuplicateFilter
 *
 * bit-aging: Each entry contains one bit used as aging flag. For new or updated entries this
 * flag is set to zero. A periodic timer of interval T_a is run. On timeout, all flags containing
 * a zero are set to one. Entries containing a flag with a value of one are removed. Maximal
 * lifetime of an entry is (T_a,2T_a). T_a can be a high value, e.g. 10 seconds. Advantages
 * of bit-aging is the low computation overhead and memory demand.
 */
template<class T, uint8_t SIZE>
class BitAgingSList: public StaticSList<T, SIZE> {
public:

	/**Remove old entries form SList*/
	void refresh() {
		uint8_t it = this->begin();
		while (it != this->end()) {
			if (age.get(it)) {
				it = this->erase(it);
			} else {
				age.set(it, true);
				it = this->next(it);
			}
		}
	}

	/**Only resets age of an entry. Does not change position of entry in arry
	 */
	void resetAge(uint8_t it) {
		if (it >= this->end()) {
			return;
		}
		age.set(it, false);
	}

	/**Treats array as queue. If full old entries are removed.
	 * Update will always put element to front an reset aging bits*/
	bool update(const T &data) {

		// try to find element
		uint8_t it = this->find(data);

		// if found, erase from list
		if (it != this->end()) {
			this->erase(it);
		}

		// if container is full, pop element
		if (this->full()) {
			this->pop_back();
		}

		// (re)add element to container, set corresponding flag to zero
		age.set(this->push_front(data), false);

		// return boolean value whether element was in list
		return it != this->end();

	}

private:
	BitVector<SIZE> age;

};

} //namespace

#endif /* BITAGINGSLIST_H_ */
