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

#ifndef S25FL_ACCESS_MSG_H_
#define S25FL_ACCESS_MSG_H_

#include <stdint.h>
#include "RemoteModule.h"
#include "Vector.h"

#define FLASH_DATA_MAX_SIZE 64

struct S25FlAccessMsg {
    S25FlAccessMsg(uint32_t addr = 0, uint16_t len = 0) :
        addr(addr),
        len(len)
    {}

    uint32_t addr;
    uint16_t len;
};

struct S25FlWriteMsg {
    S25FlWriteMsg(uint32_t addr = 0, const char * value = NULL) :
        addr(addr)
    {
        for (unsigned int i = 0; i < strlen(value) && i < FLASH_DATA_MAX_SIZE; i++) {
            data.pushBack(value[i]);
        }
    }

    uint32_t addr;
    cometos::Vector<uint8_t, FLASH_DATA_MAX_SIZE> data;
};

namespace cometos {
void serialize(ByteVector & buf, const S25FlAccessMsg & val);
void unserialize(ByteVector & buf, S25FlAccessMsg & val);

void serialize(ByteVector & buf, const S25FlWriteMsg & val);
void unserialize(ByteVector & buf, S25FlWriteMsg & val);

void serialize(ByteVector & buf, const Vector<uint8_t, FLASH_DATA_MAX_SIZE> & val);
void unserialize(ByteVector & buf, Vector<uint8_t, FLASH_DATA_MAX_SIZE> & val);

}




#endif
