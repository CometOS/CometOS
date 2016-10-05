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

#ifndef AGING_ARRAY_H
#define AGING_ARRAY_H

#include <stdint.h>
#include <assert.h>


/**Array with 4 bit aging counter for each entry. Calling update decreases array*/
template<class C, uint8_t SIZE>
class AgingArray {
public:

	AgingArray() {
		for (uint8_t i=0;i<(SIZE+1)/2;i++) {
			ttl[i]=0;
		}
	}

	void update() {
		uint8_t age;
		for (uint8_t i=0;i<SIZE;i++) {
			age=getAge(i);
			if (age==0) continue;
			age--;
			setAge(i,age);
		}
	}
	
	void reset(uint8_t index) {
		assert(index<SIZE);
		setAge(index,0x0F);
	}

	template<class T>
	bool contains(const T& value) {
		return find(value)!=SIZE;
	}
	
	template<class T>
	uint8_t find(const T& value) {
		for (uint8_t i=0;i<SIZE;i++) {
			if (value == array_[i] && getAge(i)>0) {
				return i;
			}
		}
		return SIZE;
	}

	/**This function checks if value is already in list, if not it looks for
	 * a free space, if no free space is available the oldest value is displaced. 
	 * Similar to insert function
	 */
	inline uint8_t displace(const C &value) {
		uint8_t index= find(value);
		if (index != SIZE) {
			setAge(index,0x0F);
			return index;
		}
		
		index = getOldestEntry();
		insert(value,index);
	
		return index;
	}
	
	inline void insert(const C& value, uint8_t index) {
		assert(index<SIZE);
		array_[index] = value;
		reset(index);
	}
	
	inline C& get(uint8_t index) {
		assert(index<SIZE);
		return array_[index];
	}
	
	
	/*inline const C& operator[](uint8_t index) {
		assert(index<SIZE);
		return array_[index];
	}*/

	inline uint8_t size()  {
		return size;
	} 
	
	inline uint8_t getAge(uint8_t index) {
		if (0x01&index) {
			return ttl[index>>1]>>4;
		} else {
			return ttl[index>>1]&0x0F;
		}
	}

protected:
	C array_[SIZE];
	uint8_t ttl[(SIZE+1)/2];
		
	inline uint8_t getOldestEntry() {
		uint8_t cur=SIZE;
		uint8_t index = 0;
		for (uint8_t i=0;i<SIZE;i++) {
			if ( getAge(i)<cur) {
				cur=getAge(i);
				index=i;
			}
		}
		return index;
	}
	
	
	
	
	inline void setAge(uint8_t index, uint8_t age) {
		if (0x01&index) {
			ttl[index>>1]= (ttl[index>>1]&0x0F) | (age<<4);
		} else {
			ttl[index>>1]= (ttl[index>>1]&0xF0) | (age&0x0F);
		}
	}
};




#endif //AGING_ARRAY_H
