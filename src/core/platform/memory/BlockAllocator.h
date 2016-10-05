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

#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "BitVector.h"
#include "Message.h"
#include "logging.h"
#include "memory.h"
/*MACROS---------------------------------------------------------------------*/

// size of heap in bytes (HEAP_BLOCK_SIZE * HEAP_BLOCKS)

#ifndef HEAP_BLOCK_SIZE
#define HEAP_BLOCK_SIZE		32
#endif

#define HEAP_HEADER_SIZE	8

#ifndef HEAP_BLOCKS
#define HEAP_BLOCKS			(MEMORY_HEAP_SIZE/HEAP_BLOCK_SIZE)
#endif

namespace cometos {

/**THIS CLASS IS NOT THREAD-SAFE, NEVER INVOKE MEMBERS
 * INSIDE OF AN ISR
 *
 *VIA BLOCKS and BLOCK_SIZE the performance as well as
 *the internal fragmentation can be controlled.
 *A high BLOCK_SIZE -> fast, but internal fragmentation
 *A low BLOCK_SIZE -> slow, but less internal fragmentation
 *
 * This class provides a heap for allocating memory.
 *
 * This class also performs a various of
 * integrity/consistency checks.
 *
 * It implements first-fit.
 *
 * allocate O(n)  (linear to memory size)
 * deallocate O(1)
 * getAllocatedBlocks O(n)
 *
 */
template<uint16_t BLOCKS = HEAP_BLOCKS, uint8_t BLOCK_SIZE = HEAP_BLOCK_SIZE>
class MemoryManager {
public:

	/**
	 * @return number of allocated blocks
	 */
	inline uint16_t getAllocatedBlocks() {
		return utilization.count(true);
	}

	/**
	 * @return number of total blocks
	 */
	inline uint16_t getTotalBlocks() {
		return BLOCKS;
	}

	/**
	 * Block size
	 */
	inline uint16_t getBlockSize() {
		return sizeof(uint16_t);
	}

	/**
	 * Allocates memory.
	 *
	 * @param size	size of memory block, at most 511 bytes are supported
	 */
	void* allocate(size_t size) {

		ASSERT(size > 0);
		ASSERT(size <= 255);

		void *pointer = NULL;

		// calculate required blocks (1 byte is used as header)
		uint8_t blocks = ((size+HEAP_HEADER_SIZE-1) / BLOCK_SIZE) + 1;

		uint8_t free = 0;
		for (uint16_t i = 0; i < BLOCKS; i++) {
			if (utilization.get(i)) {
				free = 0;
			} else {
				free++;
			}

			// found free block
			if (free == blocks) {
				uint16_t pos = (i + 1) - blocks;
				ASSERT(i+1 >= blocks);
				for (uint16_t j = pos; j < (pos + blocks); j++) {
					utilization.set(j, true);
				}
				// write header
				heap[pos * BLOCK_SIZE] = blocks;
				// return pointer
				pointer = &(heap[pos * BLOCK_SIZE + HEAP_HEADER_SIZE]);

				break;
			}
		}

		if (pointer == NULL) {
			ASSERT(false);
		}

		return pointer;
	}

	/**
	 * Frees allocated memory.
	 *
	 * @param pointer	valid pointer to allocated memory
	 */
	void deallocate(void* pointer) {

		ASSERT(pointer != NULL);
		ASSERT(heap < pointer);
		ASSERT(heap + BLOCKS * BLOCK_SIZE > pointer);

		uint16_t pos = ((uint16_t) ((uint8_t*) pointer - 1 - heap))
				/ BLOCK_SIZE;

		ASSERT(utilization.get(pos));
		uint8_t length = heap[pos * BLOCK_SIZE];
		for (uint16_t i = pos; i < (pos + length); i++) {

			ASSERT(utilization.get(i) == true);

			utilization.set(i, false);
		}

	}

private:
	BitVector<BLOCKS> utilization;
	uint8_t heap[BLOCKS * BLOCK_SIZE];
};

/*
 //UNIT TEST MemoryManager


 #include <assert.h>
 #include <cstdlib>
 #include <iostream>
 #include <list>



 typedef struct {
 uint16_t size;
 uint8_t* pointer;
 uint8_t offset;
 } test_t;

 // write some dummy stuff
 void fill(test_t var) {

 for (int i = 0; i < var.size; i++) {
 var.pointer[i] = i+var.offset;
 }
 }

 // check some dummy stuff
 bool check(test_t var) {
 for (int i = 0; i < var.size; i++) {
 if (var.pointer[i] != (uint8_t)(i+var.offset)) {
 return false;
 }
 }
 return true;
 }

 int main(void) {

 list<test_t> lis;

 MemoryManager<2048> mem;

 int maxAllocation = 128;

 srand(1);

 for (int i = 0; i < 500000; i++) {
 // allocate something
 if (rand() % 100 > 40) {

 int size = rand() % maxAllocation + 1;
 void * p = mem.allocate(size);
 if (p == NULL) {
 continue;
 }
 test_t test;
 test.pointer = (uint8_t*) p;
 test.size = size;
 test.offset = rand();
 //mem.print();
 lis.push_back(test);
 fill(test);

 } else { // deallocate something
 if (lis.size() == 0) {
 continue;
 }
 int sel = rand() % lis.size();
 for (list<test_t>::iterator it = lis.begin(); it != lis.end(); it++) {
 if (sel == 0) {

 assert( check(*it));
 mem.deallocate(it->pointer);
 //mem.print();
 lis.erase(it);
 break;
 }
 sel--;
 }

 }

 }

 return 0;
 }

 */
} // namepsace
#endif // BlockAllocator
