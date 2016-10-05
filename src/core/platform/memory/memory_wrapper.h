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

// to be included at the end of the actual memory allocator, which also has to
// define a local function memory which returns the allocator
// by using this not-so-beautiful inclusion of an implementation here, we
// work around having to specify the type of the allocator somewhere. We
// simply build the one we need and use it here

#include "palExec.h"
#include "cometosAssert.h"

inline void *check(void *p) {
#ifndef MEMORY_NO_EXHAUSTION_CHECK
	ASSERT(p);
#endif
	return p;
}

#ifdef MEMORY_THREAD_SAFE
void *operator new(size_t size) {
//    ASSERT(size<=255);
	void *p;
	palExec_atomicBegin();
	p = cometos::memory().allocate(size);
	palExec_atomicEnd();
	return check(p);
}

void *operator new[](size_t size) {
    ASSERT(size<=255);
	void *p;
	palExec_atomicBegin();
	p = cometos::memory().allocate(size);
	palExec_atomicEnd();
	return check(p);
}

void operator delete(void* ptr) {
    palExec_atomicBegin();
    cometos::memory().deallocate(ptr);
	palExec_atomicEnd();
}

void operator delete[](void* ptr) {
    palExec_atomicBegin();
    cometos::memory().deallocate(ptr);
	palExec_atomicEnd();
}

#else
void *operator new(size_t size) {
//    ASSERT(size<=255);
	void * p = check(cometos::memory().allocate(size));
	return p;
}

void *operator new[](size_t size) {
//    ASSERT(size<=255);
	return check(cometos::memory().allocate(size));
}

void operator delete(void* ptr) {
    cometos::memory().deallocate(ptr);
}

void operator delete[](void* ptr) {
    cometos::memory().deallocate(ptr);
}
#endif
