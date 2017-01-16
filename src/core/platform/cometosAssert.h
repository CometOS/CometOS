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
 */

#ifndef COMETOS_ASSERT_H
#define COMETOS_ASSERT_H

#include <stdint.h>
#include "palLed.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ASSERT_RESET
void resetTheNode();
#define resetNode() resetTheNode();
#else
#define resetNode()
#endif

#if defined ASSERT_SHORT || defined ASSERT_LONG
// For ASSERT_SHORT, the string created by __FILE__ has to be replaced by a
// unique FILEID by an additional build script
#define ASSERT(x) \
    if (!(x)) { \
        doAssert(__LINE__, __FILE__); \
    }

#endif

#ifdef ASSERT_SHORT
void doAssert(uint16_t line, uint16_t fileId);

#elif defined(ASSERT_LONG)
void doAssert(uint16_t line, const char* filename);

#elif defined(ASSERT_HASH)

#include <string.h>

// from http://lolengine.net/blog/2011/12/20/cpp-constant-string-hash

#define H1(s,i,x)   (x*65599u+(uint8_t)s[(i)<strlen(s)?strlen(s)-1-(i):strlen(s)])
#define H4(s,i,x)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x))))
#define H16(s,i,x)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x))))
#define H64(s,i,x)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x))))
#define H256(s,i,x) H64(s,i,H64(s,i+64,H64(s,i+128,H64(s,i+192,x))))

#define HASH(s)    ((uint32_t)(H256(s,0,0)^(H256(s,0,0)>>16)))

#define STRINGIFY(x) #x

void doAssert(uint32_t id);

#define ASSERT(x) \
    if (!(x)) { \
        doAssert(HASH(STRINGIFY(__LINE__) ":" __FILE__)); \
    }

#else

// nothing defined, just assert without any information

void doAssert(uint32_t id);

#define ASSERT(x)\
    if (!(x)) {\
        doAssert(0);\
    }

#endif

#ifdef __cplusplus
}
#endif


#endif // header define
