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

/*
 * @author Andreas Weigel
 */

#ifndef SDLIST_H_
#define SDLIST_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Derived from SList. Uses fixed size but dynamic allocation at runtime
 * to be able to influence queue size by simulation parameters rather
 * than hardcoding it.
 *
 */
template<class T>
class SDList {

public:

	SDList(uint8_t capacity) : capacity(capacity) {
		next_ = new uint8_t[capacity+1];
		previous_ = new uint8_t[capacity+1];
		data_ = new T[capacity];
		clear();
	}

	~SDList() {
		free(next_); next_ = NULL;
		free(previous_); previous_ = NULL;
		free(data_); data_ = NULL;
	}

	uint8_t erase(uint8_t it) {
		uint8_t nextIt = next(it);
		link(previous(it), nextIt);
		deallocate(it);
		return nextIt;
	}

	inline void pop_front() {
		// erase after virtual begin
		erase(begin());
	}

	inline void pop_back() {
		// erase before virtual end
		erase(previous(end()));
	}

	uint8_t insert(uint8_t it, const T& obj) {
		// allocate new block for data
		uint8_t newIt = allocate(obj);
		if (newIt == end()) {
			return end();
		}
		// linking of new blocks
		link(previous(it), newIt);
		link(newIt, it);
		return newIt;
	}

	inline uint8_t push_front(const T& obj) {
		// insert after virtual begin
		return insert(begin(), obj);
	}

	inline uint8_t push_back(const T& obj) {
		// insert before virtual end
		return insert(end(), obj);
	}

	void clear() {
		size_ = 0;

		// prepare free list
		for (uint8_t i = 0; i < capacity; i++) {
			next_[i] = i + 1;
		}
		free_ = 0;

		// virtual begin and end of list
		previous_[capacity] = capacity;
		next_[capacity] = capacity;

	}

	template<class C>
	inline uint8_t find(const C &val) {
		for (uint8_t it = begin(); it != end(); it = next(it)) {
			if (operator[](it) == val) {
				return it;
			}
		}
		return end();
	}

	inline bool empty() const {
		return size_ == 0;
	}

	inline uint8_t size() const {
		return size_;
	}

	inline uint8_t max_size() const {
		return capacity;
	}

	inline uint8_t begin() {
		return next_[capacity];
	}

	inline uint8_t end() {
		return capacity;
	}

	inline uint8_t rbegin() {
		return previous_[capacity];
	}

	inline uint8_t rend() {
		return capacity;
	}

	inline uint8_t next(uint8_t it) {
		return next_[it];
	}

	inline uint8_t previous(uint8_t it) {
		return previous_[it];
	}

	inline T &operator[](uint8_t it) {
		return data_[it];
	}

	inline T &get(uint8_t it) {
		return data_[it];
	}

private:

	uint8_t allocate(const T& obj) {
		if (size_ == capacity) {
			return end();
		}
		size_++;
		uint8_t it = free_;
		free_ = next_[it];
		data_[it] = obj;
		return it;
	}

	void deallocate(uint8_t it) {
		// push_front to free list
		//	ASSERT( size_ != 0);

		size_--;
		next_[it] = free_;
		free_ = it;
	}

	inline void link(uint8_t previous, uint8_t next) {
		next_[previous] = next;
		previous_[next] = previous;
	}

	uint8_t capacity;
	uint8_t *next_;
	uint8_t *previous_;
	T* data_;
	uint8_t free_;
	uint8_t size_;
};

#endif /* SDLIST_H_ */
