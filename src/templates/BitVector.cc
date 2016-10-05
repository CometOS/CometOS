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
 * @author Andreas Weigel
 */

#include "BitVector.h"
#include "cometosAssert.h"
#include "primitives.h"

namespace cometos {

void BitVectorBase::doAnd(const BitVectorBase& rhs) {
    ASSERT(this->len == rhs.length());
    for (uint16_t i = 0; i < BITVECTOR_BYTE_LENGTH(len); i++) {
        array[i] &= rhs.array[i];
    }
}

void BitVectorBase::doOr(const BitVectorBase& rhs) {
    ASSERT(this->len == rhs.length());
    for (uint16_t i = 0; i < BITVECTOR_BYTE_LENGTH(len); i++) {
        array[i] |= rhs.array[i];
    }
}

/**
 * Counts occurrence of a specific value.
 *
 * @param value the boolean value which should be counted
 * @return number of occurrences
 */
uint16_t BitVectorBase::count(bool value) const {
    uint16_t counter = 0;
    for (uint16_t i = 0; i < len; i++) {
        if (get(i) == value) {
            counter++;
        }
    }
    return counter;
}

void BitVectorBase::fill(uint8_t* arr) {
    for (uint16_t i = 0; i < BITVECTOR_BYTE_LENGTH(len); i++) {
        array[i] = arr[i];
    }
}

void BitVectorBase::fill(bool value) {
    for (uint16_t i = 0; i < BITVECTOR_BYTE_LENGTH(len); i++) {
        array[i] = value * 0xFF;
    }
}

void BitVectorBase::set(uint16_t pos, bool value) {
    if (pos >= len) {
        return;
    }
    if (value) {
        array[pos >> 3] |= 1 << (pos & 0x07);
    } else {
        array[pos >> 3] &= ~(1 << (pos & 0x07));
    }

}

bool BitVectorBase::get(uint16_t pos) const {
    if (pos >= len) {
        return false; // undefined
    }
    if (array[pos >> 3] & (1 << (pos & 0x07))) {
        return true;
    } else {
        return false;
    }

}

uint16_t BitVectorBase::length() const {
    return len;
}
uint16_t BitVectorBase::getByteLenth() const {
    return BITVECTOR_BYTE_LENGTH(len);
}


void serialize(ByteVector& buffer,
        const BitVectorBase& value) {
    for (uint16_t i = 0; i < value.getByteLenth(); i++) {
        serialize(buffer, value.array[i]);
    }

}

void unserialize(ByteVector& buffer, BitVectorBase& value) {
    uint8_t val;
    for (uint16_t i = value.getByteLenth(); i > 0; i--) {
        unserialize(buffer, val);
        value.array[i - 1] = val;
    }
}


} // namespace
