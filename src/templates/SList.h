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

#ifndef SLIST_H_
#define SLIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

namespace cometos {

/**
 * New efficient list implementation with static allocation. This class
 * should replace old implementation
 * Internal free list is handled as single connected list
 *
 * Example:
 * StaticSList<uint16_t,10> l;
 * l.push_front(1)
 * l.push_front(3)
 * l.push_front(4)
 *
 * for (uint8_t i=l.begin(); i!=l.end(); i=l.next(i)) {
 * 		do something
 * }
 *
 *
 *
 */
template<class T>
class SListBase {
public:

	// TODO,  ADD class for iterator (make ++/--  private, allow passing reference to next(), previous )
	SListBase(T* baseArray,
	          uint8_t* nextArray,
	          uint8_t* prevArray,
	          uint8_t size) :
	    data_(baseArray),
	    next_(nextArray),
	    previous_(prevArray),
	    MAX_SIZE(size)
    {
		clear();
	}

protected:
	SListBase(const SListBase<T>& rhs,
	          T* dataArray = NULL,
	          uint8_t* nextArray = NULL,
	          uint8_t* prevArray = NULL,
	          uint8_t  size = 0) :
	              data_(dataArray),
	              next_(nextArray),
	              previous_(prevArray),
	              MAX_SIZE(size)
	{
	    *this = rhs;
	}

	SListBase& operator=(const SListBase& rhs) {
	    if (this != &rhs) {
	        clear();
	        for (uint8_t it=rhs.begin(); it != rhs.end(); it=rhs.next(it)) {
	            this->push_back(rhs.get(it));
	        }
	        return *this;
	    }
	    return *this;
	}

public:
	virtual ~SListBase() {}


	uint8_t erase(uint8_t it) {
		if (it == end()) {
			return it;
		}
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
		for (uint8_t i = 0; i < MAX_SIZE; i++) {
			next_[i] = i + 1;
		}
		free_ = 0;

		// virtual begin and end of list
		previous_[MAX_SIZE] = MAX_SIZE;
		next_[MAX_SIZE] = MAX_SIZE;

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

	inline bool full() const {
		return size_ == MAX_SIZE;
	}

	inline uint8_t max_size() const {
		return MAX_SIZE;
	}

	inline uint8_t begin() const {
		return next_[MAX_SIZE];
	}

	inline uint8_t end() const {
		return MAX_SIZE;
	}

	inline uint8_t rbegin() {
		return previous_[MAX_SIZE];
	}

	inline uint8_t rend() {
		return MAX_SIZE;
	}

	inline uint8_t next(uint8_t it) const {
		return next_[it];
	}

	inline uint8_t previous(uint8_t it) {
		return previous_[it];
	}

	inline T &operator[](uint8_t it) {
		return data_[it];
	}

	inline const T &get(uint8_t it) const {
		return data_[it];
	}

protected:
	uint8_t*& getNextArray() {
	    return next_;
	}

	uint8_t*& getPrevArray() {
	    return previous_;
	}

	T*& getDataArray() {
	    return data_;
	}

private:
	uint8_t allocate(const T& obj) {
		if (size_ == MAX_SIZE) {
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

	T* data_;
	uint8_t* next_;
	uint8_t* previous_;
	uint8_t free_;
	uint8_t size_;
	uint8_t MAX_SIZE;
};


template <class T, uint8_t SIZE>
class StaticSList : public SListBase<T> {
public:
    StaticSList() :
        SListBase<T>(data_, next_, previous_, SIZE)
    {
    }

    StaticSList(const StaticSList<T, SIZE>& rhs) :
        SListBase<T>(rhs, data_, next_, previous_, SIZE)
    {
        // need to call assignment here (again), because
        // data_ array is initialized by calling T's constructor,
        // which means everything done in base class is undone
        // TODO can we somehow prevent the double copy here?
        *this = rhs;
    }

    StaticSList& operator=(const StaticSList& rhs) {
        if (this != &rhs) {
            this->SListBase<T>::operator=(rhs);
            return *this;
        }
        return *this;
    }

    ~StaticSList() {
        // nothing, everything statically allocated
    }

private:
    uint8_t next_[SIZE + 1];
    uint8_t previous_[SIZE + 1];
    T data_[SIZE];
};

template <class T>
class DynSList : public SListBase<T> {
public:
    DynSList(uint8_t size) :
        SListBase<T>(new T[size],
                 new uint8_t[size+1],
                 new uint8_t[size+1],
                 size)
    {}

    virtual ~DynSList() {
        uint8_t*& next = this->getNextArray();
        uint8_t*& prev = this->getPrevArray();
        T*& data = this->getDataArray();
        delete[] next;
        delete[] prev;
        delete[] data;
    }
};

}

#endif /* SList_H_ */
