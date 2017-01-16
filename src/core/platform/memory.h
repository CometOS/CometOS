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
 * @author Stefan Unterschütz
 *
 * Optionally you can use the following switches:
 *
 * #define MEMORY_THREAD_SAFE	      -  memory operations are atomic
 *
 * #define MEMORY_NO_EXHAUSTION_CHECK - no assert in case of an exhausted
 * 										memory (NULL is returned)
 *
 * The following implementations for the memory allocation exist:
 *
 * <li> BlockAllocator: paging algorithm
 * <li> SurmAllocator: multiple pool allocation (dynamic pool management)
 *
 * Currently it is recommend to use the SurmAllocator.
 */

#ifndef MEMORY_H_
#define MEMORY_H_

/*MACROS---------------------------------------------------------------------*/

#ifndef MEMORY_HEAP_SIZE
#define MEMORY_HEAP_SIZE	 8192
#endif


/*INCLUDES-------------------------------------------------------------------*/


#include <stdlib.h>
#include <stdint.h>


/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {
// return utilization in percent
uint8_t heapGetUtilization();

}


void *operator new(size_t size);

void *operator new[](size_t size);

void operator delete(void* ptr);

void operator delete[](void* ptr);


#endif /* MEMORY_H_ */
