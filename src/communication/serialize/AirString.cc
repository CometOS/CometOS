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

#include "primitives.h"
#include "AirString.h"
#include <string.h>
#include "OutputStream.h"

namespace cometos {

const uint8_t AirString::MAX_LEN;

AirString::AirString(const char* str_) {
	setStr(str_);
}

void AirString::setStr(const char* str_) {
	strncpy(str, str_, MAX_LEN);

	// always mark last element as 0
	str[MAX_LEN] = 0;
}

uint8_t AirString::getLen() const {
    uint8_t i;
    for (i=0; str[i] != 0 && i<=AirString::MAX_LEN; i++) {

    }
    return i;
}

const char * AirString::getStr() const {
	return str;
}

char * AirString::getStr_unsafe() {
	return str;
}

void serialize(ByteVector& buffer, const AirString& value) {
	uint8_t i = 0;
	const char* str=value.getStr();
	while (str[i] != 0 && i < AirString::MAX_LEN) {
		serialize(buffer, str[i]);
		i++;
	}
	serialize(buffer, i);
}

void unserialize(ByteVector& buffer, AirString& value) {
	uint8_t len;
	unserialize(buffer, len); // get length of string
	ASSERT(len<=AirString::MAX_LEN);

	char* str=value.getStr_unsafe();

	str[len] = 0;
	for (int8_t i = len - 1; i >= 0; i--) {
		unserialize(buffer, str[i]);
	}
}

} /* namespace cometos */
