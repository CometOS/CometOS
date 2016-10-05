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

#include "S25FlAccessMsg.h"

namespace cometos {

void serialize(ByteVector & buf, const S25FlAccessMsg & val) {
    serialize(buf, val.addr);
    serialize(buf, val.len);
}

void unserialize(ByteVector & buf, S25FlAccessMsg & val) {
    unserialize(buf, val.len);
    unserialize(buf, val.addr);
}

void serialize(ByteVector & buf, const S25FlWriteMsg & val) {
    serialize(buf, val.addr);
    serialize(buf, val.data);
}

void unserialize(ByteVector & buf, S25FlWriteMsg & val) {
    unserialize(buf, val.data);
    unserialize(buf, val.addr);
}

void serialize(ByteVector & buf, const Vector<uint8_t, FLASH_DATA_MAX_SIZE> & val) {
    uint8_t size = val.getSize();
    for (uint8_t i = size; i > 0; i--) {
        serialize(buf, val[i - 1]);
    }
    serialize(buf, size);
}
void unserialize(ByteVector & buf, Vector<uint8_t, FLASH_DATA_MAX_SIZE> & val) {
    val.clear();
    uint8_t size;
    uint8_t item;
    unserialize(buf, size);
    for (uint8_t i = 0; i < size; i++) {
        unserialize(buf, item);
        val.pushBack(item);
    }
}

}


