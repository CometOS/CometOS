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

#ifdef MEMORY_THREAD_SAFE
#include "atomic.h"
#endif

#include "MallocAllocator.h"
#include "stdlib.h"
#include "memory.h"

extern char * __malloc_heap_end;
extern char * __malloc_heap_start;

namespace cometos {

// ensures that MemoryManager is initialized
static inline MallocAllocator &memory() {
    static MallocAllocator mem;
    return mem;
}

void * MallocAllocator::allocate(size_t size) {
    void * res = malloc(size);
    if (((uintptr_t) res) > pMax) {
        pMax = (uintptr_t) res;
    }
    return res;
}

void MallocAllocator::deallocate(void * pointer) {
    free(pointer);
}

uintptr_t MallocAllocator::pMax = (uintptr_t) ::__malloc_heap_start;

uint8_t heapGetUtilization() {
    return ((uint32_t) (MallocAllocator::pMax - (uintptr_t) __malloc_heap_start)) * 100 / ((uintptr_t)::__malloc_heap_end - (uintptr_t)::__malloc_heap_start);
}

} // namespace

#include "memory_wrapper.h"
