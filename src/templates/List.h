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

#ifndef LIST_H_
#define LIST_H_

#include <stdlib.h>


#define LIST_DEFAULT_SIZE	10

/**
 * This class provides part of functionality like the STL container "list".
 * Iteration and field access are similar to the STL class. Only the size
 * is restricted and has to be assigned ate the instantiation (max 254). Thus this
 * class is completely static and uses no dynamic memory allocation.
 *
 */
template<class T, uint8_t SIZE = LIST_DEFAULT_SIZE>
class List {
public:

	class iterator {
		friend class List<T, SIZE> ;
	public:
		iterator(List<T, SIZE> *instance = NULL) :
			previous(SIZE), current(SIZE), instance(instance) {
		}

		inline T *operator->() {
			// no check for valid iterator, should be done by user
			return &instance->data[current];
		}

		inline T &operator*() {
			// no check for valid iterator, should be done by user
			return instance->data[current];
		}

		inline iterator& operator++(int) {

			// restart if end is reached
			if (current == SIZE) {
				previous = SIZE;
				current = instance->begin_.current;
			} else {
				previous = current;
				current = instance->pointer[current];
			}
			return *this;
		}

		inline bool operator==(const iterator &it) const {
			// TODO check if instance is the same
			return current == it.current;
		}

		inline bool operator!=(const iterator &it) const {
			// TODO check if instance is the same
			return current != it.current;
		}

	private:
		uint8_t previous;
		uint8_t current;
		List<T, SIZE> *instance;
	};

	List() :
		size_(0), begin_(this), end_(this) {
		clear();
	}

	void clear() {
		begin_.previous = SIZE;
		begin_.current = SIZE;
		end_.previous = SIZE;
		end_.current = SIZE;

		size_ = 0;

		// initialize free list
		free_ = 0;
		for (uint8_t i = 0; i < SIZE; i++) {
			pointer[i] = i + 1;
		}
	}

	uint8_t allocate() {
		if (free_ == SIZE) {
			return SIZE;
		} else {
			uint8_t temp = free_;
			free_ = pointer[free_];
			size_++;
			return temp;
		}
	}

	template<class C>
	iterator find(const C &val) {
		for (iterator it = this->begin(); it != this->end(); it++) {
			if ((*it) == val) {
				return it;
			}
		}
		return end_;
	}

	void deallocate(uint8_t i) {
		pointer[i] = free_;
		free_ = i;
		size_--;
	}

	void push_front(const T& val) {

		uint8_t i = allocate();
		if (i == SIZE) {
			return;
		}

		// set values
		pointer[i] = begin_.current;
		begin_.current = i;
		data[i] = val;

		// if list was empty update end iterator
		if (end_.previous == SIZE) {
			end_.previous = i;
		}
	}

	void push_back(const T& val) {
		uint8_t i = allocate();
		if (i == SIZE) {
			return;
		}

		// set values
		if (begin_.current == SIZE) {
			begin_.current = i;
		} else {
			pointer[end_.previous] = i;
		}

		pointer[i] = SIZE;
		end_.previous = i;
		data[i] = val;

	}

	inline const iterator &begin() {
		return begin_;
	}

	inline const iterator &end() {
		return end_;
	}

	inline uint8_t size() {
		return size_;
	}

	void erase(iterator &pos) {
		if (pos==end_) {
			return;
		}

		if (pos.previous != SIZE) {
			pointer[pos.previous] = pointer[pos.current];
		} else {
			begin_.current = pointer[pos.current];
		}

		if (pointer[pos.current] == SIZE) {
			end_.previous = pos.previous;
		}

		deallocate(pos.current);

		pos.current = pos.previous;
		pos.previous = SIZE;
	}

private:
	uint8_t pointer[SIZE];
	T data[SIZE];
	uint8_t size_;
	iterator begin_;
	iterator end_;

	uint8_t free_;
};

#endif /* LIST_H_ */
