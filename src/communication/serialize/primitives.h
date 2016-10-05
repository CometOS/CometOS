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
 * @author Andreas Weigel, Stefan Unterschütz
 */


#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_

#include <Vector.h>

namespace cometos {
void serialize(ByteVector& buffer, const bool& value);

void unserialize(ByteVector& buffer, bool& value);

void serialize(ByteVector& buffer, const uint8_t&value);

void unserialize(ByteVector& buffer, uint8_t&value);

void serialize(ByteVector& buffer, const int8_t&value);

void unserialize(ByteVector& buffer, int8_t&value);

void serialize(ByteVector& buffer, const char& value);

void unserialize(ByteVector& buffer, char& value);

void serialize(ByteVector& buffer, const int16_t&value);

void unserialize(ByteVector& buffer, int16_t&value);

void serialize(ByteVector& buffer, const uint16_t&value);

void unserialize(ByteVector& buffer, uint16_t&value);

void serialize(ByteVector& buffer, const uint32_t&value);

void unserialize(ByteVector& buffer, uint32_t&value);

void serialize(ByteVector& buffer, const int32_t&value);

void unserialize(ByteVector& buffer, int32_t&value);

void serialize(ByteVector& buffer, const uint64_t&value);

void unserialize(ByteVector& buffer, uint64_t&value);

void serialize(ByteVector& buffer, const int64_t&value);

void unserialize(ByteVector& buffer, int64_t&value);

void serialize(ByteVector& buffer, const char* str, uint8_t len);


static inline void serializeBool(ByteVector& buffer, const bool val) {
	bool value = val;
	serialize(buffer, value);
}

static inline bool unserializeBool(ByteVector& buffer) {
	bool value = 0;
	unserialize(buffer, value);
	return value;
}

static inline void serializeU8(ByteVector& buffer, const int val) {
	uint8_t value = val;
	serialize(buffer, value);
}

static inline int unserializeU8(ByteVector& buffer) {
	uint8_t value = 0;
	unserialize(buffer, value);
	return value;
}

static inline void serializeU16(ByteVector& buffer, const int val) {
	uint16_t value = val;
	serialize(buffer, value);
}

static inline int unserializeU16(ByteVector& buffer) {
	uint16_t value = 0;
	unserialize(buffer, value);
	return value;
}


static inline void serializeU32(ByteVector& buffer, const int val) {
	uint32_t value = val;
	serialize(buffer, value);
}

static inline int unserializeU32(ByteVector& buffer) {
	uint32_t value = 0;
	unserialize(buffer, value);
	return value;
}

static inline void serializeU64(ByteVector& buffer, const uint64_t val) {
	uint64_t value = val;
	serialize(buffer, value);
}

static inline uint64_t unserializeU64(ByteVector& buffer) {
	uint64_t value = 0;
	unserialize(buffer, value);
	return value;
}


}

#endif /* PRIMITIVES_H_ */
