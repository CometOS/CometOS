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
 * @author Andreas Weigel, Stefan Unterschuetz
 */

#include "primitives.h"

namespace cometos {

void serialize(ByteVector& buffer, const bool& value) {
	if (value) {
		buffer.pushBack(1);
	} else {
		buffer.pushBack(0);
	}
}

void unserialize(ByteVector& buffer, bool& value) {
	uint8_t truth;
	truth = (uint8_t) buffer.popBack();
	value = truth == 1 ? true : false;
}

void serialize(ByteVector& buffer, const uint8_t& value) {
	buffer.pushBack(value);
}

void unserialize(ByteVector& buffer, uint8_t& value) {
	value = buffer.popBack();
}

void serialize(ByteVector& buffer, const int8_t& value) {
    buffer.pushBack(value);
}

void unserialize(ByteVector& buffer, int8_t& value) {
    value = buffer.popBack();
}

void serialize(ByteVector& buffer, const char& value) {
	buffer.pushBack((const uint8_t) value);
}

void unserialize(ByteVector& buffer, char& value) {
	value = (const char) buffer.popBack();
}

void serialize(ByteVector& buffer, const int16_t& value) {
    buffer.pushBack((value >> 8) & 0xFF);
    buffer.pushBack(value & 0xFF);
}

void unserialize(ByteVector& buffer, int16_t& value) {
    value = buffer.popBack();
    value |= ((int16_t) buffer.popBack() << 8);
}

void serialize(ByteVector& buffer, const uint16_t& value) {
	buffer.pushBack((value >> 8) & 0xFF);
	buffer.pushBack(value & 0xFF);
}

void unserialize(ByteVector& buffer, uint16_t& value) {
	value = buffer.popBack();
	value |= ((uint16_t) buffer.popBack() << 8);
}

void serialize(ByteVector& buffer, const uint32_t& value) {
	buffer.pushBack((value >> 24) & 0xFF);
	buffer.pushBack((value >> 16) & 0xFF);
	buffer.pushBack((value >> 8) & 0xFF);
	buffer.pushBack(value & 0xFF);
}

void unserialize(ByteVector& buffer, uint32_t& value) {
	value = buffer.popBack();
	value |= ((uint32_t) buffer.popBack()) << 8;
	value |= ((uint32_t) buffer.popBack()) << 16;
	value |= ((uint32_t) buffer.popBack()) << 24;
}

void serialize(ByteVector& buffer, const int32_t& value) {
    buffer.pushBack((value >> 24) & 0xFF);
    buffer.pushBack((value >> 16) & 0xFF);
    buffer.pushBack((value >> 8) & 0xFF);
    buffer.pushBack(value & 0xFF);
}
void unserialize(ByteVector& buffer, int32_t& value) {
    value = buffer.popBack();
    value |= ((int32_t) buffer.popBack() << 8);
    value |= ((int32_t) buffer.popBack() << 16);
    value |= ((int32_t) buffer.popBack() << 24);
}


void serialize(ByteVector& buffer, const uint64_t& value) {
	for (int i = 0; i < 64; i += 8) {
		serialize(buffer, (uint8_t) (value >> i));
	}
}
void unserialize(ByteVector& buffer, uint64_t& value) {
	value = 0;
	uint8_t tmp = 0;
	for (int i = 0; i < 64; i += 8) {
		unserialize(buffer, tmp);
		value = (uint64_t) (((value << 8) | ((uint64_t) tmp)));
	}
}

void serialize(ByteVector& buffer, const int64_t& value) {
	serialize(buffer,(uint64_t) value);
}

void unserialize(ByteVector& buffer, int64_t& value) {
	uint64_t tmp = 0;
	unserialize(buffer,tmp);
	value = tmp;
}

void serialize(ByteVector& buffer, const char* str, uint8_t len) {
    for (uint16_t i = 0; i < len; i++) {
            serialize(buffer, str[i]);
    }
}

}
