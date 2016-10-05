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

#ifndef DLIST_H_
#define DLIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define LIST_DEFAULT_CAPACITY	30

/**
 * Lists are a kind of sequence containers. As such, their elements are
 * ordered following a linear sequence.
 * This DList implementation is in most parts equivalent to the C++ STL
 * DList.
 *
 * Uses dynamic memory allocation.
 * TODO: UNIT TEST
 */
template<class T>
class DList {
public:
	struct DListEntry_t {
		DListEntry_t(DListEntry_t * next, DListEntry_t * previous,
				const T & obj) :
			next(next), previous(previous), obj(obj) {
		}

		DListEntry_t *next;
		DListEntry_t *previous;
		T obj;
	};

	class iterator {
		friend class DList<T> ;
	private:
		DListEntry_t *pointer;
	public:
		iterator(DListEntry_t *pointer = NULL) :
			pointer(pointer) {
		}

		inline T *operator->() {
			// no check for valid iterator, should be done by user
			return &pointer->obj;
		}

		inline T &operator*() {
			// no check for valid iterator, should be done by user
			return pointer->obj;
		}

		inline iterator& operator++(int) {
			if (pointer != NULL) {
				pointer = pointer->next;
			}
			return *this;
		}

		inline iterator& operator--(int) {
			if (pointer != NULL) {
				pointer = pointer->previous;
			}
			return *this;
		}


		inline bool operator==(const iterator &it) const {
			return pointer == it.pointer;
		}

		inline bool operator!=(const iterator &it) const {
			return pointer != it.pointer;
		}
	};

	/**
	 * Constructs a new linked DList.
	 *
	 * @param capacity	maximum number of elements in DList container.
	 *
	 */
	DList(uint8_t capacity = LIST_DEFAULT_CAPACITY) :
		begin_(NULL), end_(NULL), size_(0), max_size_(capacity) {
	}

	/**
	 * Destructor removes all elements stored DList. Destructor is called for each.
	 */
	~DList() {
		clear();
	}

	/**
	 * Removes a single element from the DList.
	 *
	 * @param position 	iterator pointing to an element to be removed from the DList. The
	 * 					iterator gets invalid after the call of this method.
	 *
	 * @return 	iterator pointing to the new location of the element that followed the
	 * 			last element erased by the function call, which is the DList end if the
	 * 			operation erased the last element in the sequence.
	 *
	 */
	iterator erase(iterator &position) {
		// pointer is invalid
		if (position.pointer == NULL) {
			return iterator(NULL);
		}

		if (position.pointer == begin_) {
			pop_front();
			return begin();
		}

		DListEntry_t *previous = position.pointer->previous;
		DListEntry_t *next = position.pointer->next;

		link(previous, next); // connect both

		delete position.pointer;
		size_--;
		return iterator(next);
	}

	/**
	 * Inserts a new element at the beginning of the DList, right before its current
	 * first element. The content of this new element is initialized to a copy of obj.
	 *
	 * @param obj Value to be copied to the new element.
	 *
	 */
	void push_front(const T& obj) {
		if (size_ >= max_size_) {
			return;
		}

		begin_ = new DListEntry_t(begin_, NULL, obj);
		if (end_ == NULL) {
			end_ = begin_;
		}
		size_++;
		link(begin_, begin_->next);
	}

	/**
	 * Inserts a new element at the end of the DList, right after its current
	 * last element. The content of this new element is initialized to a copy of obj.
	 *
	 * @param obj Value to be copied to the new element.
	 *
	 */
	void push_back(const T& obj) {
		if (size_ >= max_size_) {
			return;
		}

		end_ = new DListEntry_t(NULL, end_, obj);
		if (begin_ == NULL) {
			begin_ = end_;
		}
		size_++;
		link(end_->previous, end_);
	}

	/**
	 * The list container is extended by inserting new elements before the element
	 * at position.
	 *
	 * @param iterator Position in the container where the new elements are inserted.
	 * @param obj Value to be copied to the new element.
	 * @return  iterator that points to the newly inserted element.
	 */
	iterator insert(iterator& position, const T& obj) {

		// no valid iterator
		if (position.pointer == NULL) {
			return end();
		}

		if (position.pointer == begin_) {
			push_front(obj);
			return begin();
		}

		DListEntry_t *previous = position.pointer->previous;
		DListEntry_t *current = new DListEntry_t(begin_, NULL, obj);
		size_++;

		link(previous, current);
		link(current, position.pointer);

		return iterator(current);
	}

	/**
	 * Removes the first element in the DList container. This calls the removed
	 * element's destructor.
	 */
	void pop_front() {
		if (begin_ == NULL) {
			return;
		}

		if (end_ == begin_) {
			end_ = NULL;
		}

		DListEntry_t *next = begin_->next;
		delete begin_;
		size_--;
		begin_ = next;
		if (begin_ != NULL) {
			begin_->previous = NULL;
		}

	}

	/**
	 * Removes the last element in the DList container.  This calls the removed
	 * element's destructor.
	 *
	 */
	void pop_back() {
		if (end_ == NULL) {
			return;
		}

		if (end_ == begin_) {
			begin_ = NULL;
		}

		DListEntry_t *previous = end_->previous;
		delete end_;
		size_--;
		end_ = previous;
		if (end_ != NULL) {
			end_->next = NULL;
		}

	}

	/**
	 * All the elements in the DList container are dropped: their destructors are
	 * called, and then they are removed from the DList container, leaving it with
	 * a size of 0.
	 */
	void clear() {
		while (!empty()) {
			pop_front();
		}
	}

	/**
	 * @return whether the DList container is empty, i.e. whether its size is 0.
	 */
	inline bool empty() const {
		return size_ == 0;
	}

	/**
	 * @return  the number of elements in the container.
	 */
	uint8_t size() const {
		return size_;
	}

	/**
	 * @return the maximum number of elements that the DList container can hold.
	 */
	uint8_t max_size() const {
		return max_size_;
	}

	/**
	 * @return an iterator referring to the first element in the DList container.
	 * */
	iterator begin() {
		return iterator(begin_);
	}

	iterator end() {
		return iterator(NULL);
	}

private:

	void link(DListEntry_t *previous, DListEntry_t *next) {

		if (previous != NULL) {
			previous->next = next;
		}
		if (next != NULL) {
			next->previous = previous;
		}
	}

	DListEntry_t *begin_;
	DListEntry_t *end_;

	uint8_t size_;
	uint8_t max_size_;
};

/*
 #include <iostream>
 #include <DList>

 #include <assert.h>
 #include "DList.h"


 class TestObj {
 public:
 TestObj(int value) :
 value(value) {
 ref++;
 printf("create %d\n", ref);
 }

 TestObj(const TestObj& copy) {
 *this = copy;
 }

 void operator=(const TestObj& copy) {
 this->value = copy.value;
 ref++;

 }

 bool operator==(TestObj& obj) {
 return value == obj.value;
 }

 bool operator==(int value) {
 return this->value == value;
 }

 ~TestObj() {
 ref--;
 printf("delete %d\n", ref);
 }

 int value;
 static int ref;
 };

 int TestObj::ref = 0;

 int main() {
 unsigned int i;
 DList<TestObj> myDList;

 // set some values:
 for (i = 1; i < 10; i++)
 myDList.push_back(TestObj(i * 10));

 // 10 20 30 40 50 60 70 80 9
 DList<TestObj>::iterator it = myDList.begin();

 while (it != myDList.end()) {
 cout << it->value << endl;
 if (*it == 60) {
 it = myDList.erase(it);
 } else {
 it++;
 }
 }

 it = myDList.begin();
 while (it != myDList.end()) {
 cout << it->value << endl;
 if (*it == 60) {
 it = myDList.erase(it);
 } else {
 it++;
 }
 }
 myDList.clear();
 assert(TestObj::ref == 0);
 return 0;
 }
 */

#endif /* DLIST_H_ */
