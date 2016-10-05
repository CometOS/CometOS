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

#ifndef QUEUE
#define QUEUE

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

namespace cometos  {

/**Generic class for a FIFO Queue
 * Note that class is not thread safe.
 *
 * @author Stefan Unterschütz
 */
template<class C, uint8_t SIZE>
class Queue {
public:
	Queue() :
		head(0), size(0) {
	}

	inline void clear() {
		head = 0;
		size = 0;
	}

	/**
	 * @return number of used places (queue length)
	 */
	inline uint8_t used() {
		return size;
	}

	/**
	 *  @return number of free places
	 */
	inline uint8_t free() {
		return (SIZE - size);
	}

	/**
	 *@return <code>true</code> if queue is empty
	 */
	inline bool empty() {
		return (size == 0);
	}

	/**
	 *@return <code>true</code> if queue is full
	 */
	inline bool full() {
		return (size == SIZE);
	}

	/**
	 * @return first element in queue, if queue is empty the return value is not defined
	 */
	inline C &front() {
	    ASSERT(size>0);
		return array[head];
	}

	/**Removes head from queue
	 *
	 * @return <code>true</code> if operation succeed.
	 */
	bool pop() {
		if (empty()) {
			return false;
		}
		size--;
		head++;
		if (head == SIZE) {
			head = 0;
		}
		return true;
	}

	/**Removes element and stores them in array.
	 *
	 * @return number of poped/stored elements
	 * */
	uint8_t popArray(C *data, uint8_t length) {

		if (used() < length) {
			length = used();
		}

		if (0 == length) {
			return 0;
		}

		size -= length;

		if (head + length <= SIZE) {
			memcpy(data, &array[head], sizeof(C) * length);
			head++;
			if (head == SIZE) {
				head = 0;
			}
		} else {
			uint8_t part = (SIZE - head);
			memcpy(data, &array[head], sizeof(C) * part);
			memcpy(data + part, array, sizeof(C) * (length - part));
			head = (length - part);
		}

		return length;
	}

	/**Appends an element to queue
	 *
	 * @return  <code>true</code> if operation succeed.
	 *
	 */
	bool push(const C &val) {
		uint8_t tail = head + size;

		if (tail >= SIZE) {
			tail -= SIZE;
		}
		if (full()) {
			return false;
		}

		size++;
		array[tail] = val;

		return true;

	}

	/**Removes first occurence of given parameter. Allows different
	 * types for comparison with list entries.
	 *
	 *TODO TEST ME
	 * @return <code>true</code> if one element was found
	 */
	template<class T>
	bool remove(const T &val) {
		if (empty()) {
			return false;
		}

		uint8_t pos = head;
		uint8_t pos2;
		uint8_t count = 0;

		while (count < size) {
			if (array[pos] == val) {
				break;
			}
			count++;
			pos++;
			if (pos >= SIZE) {
				pos -= SIZE;
			}
		}

		if (count == size) {
			return false;
		}

		// found item
		size--;

		// rise in rank
		while (count < size) {

			pos2 = pos + 1;
			if (pos2 >= SIZE) {
				pos2 -= SIZE;
			}
			array[pos] = array[pos2];

			count++;
			pos++;
			if (pos >= SIZE) {
				pos -= SIZE;
			}
		}
		return true;

	}

	/**Checks if given element is already in  queue
	 *
	 * TODO test me
	 * */
	template<class T>
	bool consists(const T &val) {
		if (empty()) {
			return false;
		}
		uint8_t pos = head;
		uint8_t count = 0;
		while (count < size) {

			if (array[pos] == val) {
				return true;
			}
			count++;
			pos++;
			if (pos >= SIZE) {
				pos -= SIZE;
			}
		}
		return false;
	}

	/**Allows to access by key if comparison between them is defined
	 *
	 * @return valid pointer to first occurrence of instance, or zero if entry not exist
	 * TODO test me
	 * */
	template<class T>
	C* get(const T &val) {
		if (empty()) {
			return (C *) NULL;
		}
		uint8_t pos = head;
		uint8_t count = 0;
		while (count < size) {

			if (array[pos] == val) {
				return &array[pos];
			}
			count++;
			pos++;
			if (pos >= SIZE) {
				pos -= SIZE;
			}
		}
		return NULL;
	}

	/**Appends an array to queue.
	 *
	 * @return number of stored elements
	 * */
	uint8_t pushArray(C *data, uint8_t length) {
		uint8_t tail = head + size;
		if (tail >= SIZE) {
			tail -= SIZE;
		}

		if (free() < length) {
			length = free();
		}

		if (0 == length) {
			return 0;
		}

		size += length;

		if (tail + length <= SIZE) {
			memcpy(&array[tail], data, sizeof(C) * length);
		} else {
			uint8_t part = (SIZE - tail);
			memcpy(&array[tail], data, sizeof(C) * part);
			memcpy(array, data + part, sizeof(C) * (length - part));
		}
		return length;

	}

	uint8_t getSize() {
		return size;
	}

private:
	uint8_t head;
	uint8_t size;

	C array[SIZE];
};


}
/*

 // unit test
 static bool testQueue() {
 #define TEST(x) if ( false==(x)) return false

 int val;
 int array[]={10,11,12,13,14,15,16};
 Queue< int, 5> queue;

 // test basic operations
 TEST(queue.empty());
 TEST(queue.full()==false);
 TEST(queue.free()==5);
 TEST(queue.used()==0);

 // test push pop
 TEST(queue.push(val=1));
 TEST(queue.push(val=2));

 TEST(1==queue.front());
 TEST(queue.pop());
 TEST(2==queue.front());

 TEST(queue.free()==4);
 TEST(queue.used()==1);

 for (int i=3;i<7;i++) {
 TEST(queue.push(i));
 }
 TEST(false==queue.push(val=1));
 TEST(queue.free()==0);
 TEST(queue.used()==5);

 for (int i=2;i<7;i++) {
 TEST(i==queue.front());
 TEST(	cout<<testQueue()<<endl;queue.pop());
 }
 TEST(queue.free()==5);
 TEST(queue.used()==0);

 // test array push
 TEST(5==queue.pushArray(array,7));

 TEST(queue.free()==0);
 TEST(queue.used()==5);
 TEST(10==queue.front());
 TEST(queue.pop());
 TEST(11==queue.front());
 TEST(queue.pop());

 TEST(2==queue.pushArray((array+5),2));
 TEST(queue.used()==5);
 for (int i=12;i<17;i++) {
 TEST(i==queue.front());
 TEST(queue.pop());
 }

 TEST(false==queue.pop());
 TEST(queue.free()==5);
 TEST(queue.used()==0);


 // test array pop
 TEST(5==queue.pushArray(array+2,7));

 TEST(1==queue.popArray(array,1));
 TEST(queue.used()==4);
 TEST(12==*array);
 TEST(4==queue.popArray(array,6));
 TEST(queue.used()==0);
 for (int i=13;i<17;i++) {
 TEST(i==array[i-13]);
 }


 return true;
 }
 */

#endif
