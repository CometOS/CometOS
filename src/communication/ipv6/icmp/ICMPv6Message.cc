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
 * @author: Martin Ringwelski
 */

#include "ICMPv6Message.h"

namespace cometos_v6 {

const headerType_t ICMPv6Message::HeaderNumber;
const uint8_t ICMPv6Message::FIXED_SIZE;

ICMPv6Message::ICMPv6Message (uint8_t type, uint8_t code) :
    FollowingHeader(ICMPv6Message::HeaderNumber),
    type(type),
    code(code),
    checksum(0),
    data(NULL),
    length(0),
    responsibleForData(false)
{
    additionalFields[0] = 0;
    additionalFields[1] = 0;
}

ICMPv6Message::~ICMPv6Message() {
    if (responsibleForData) {
        delete [] data;
    }
}

ICMPv6Message * ICMPv6Message::clone() const {
    ICMPv6Message * copy = new ICMPv6Message(this->type, this->code);
    copy->checksum = this->checksum;
    copy->responsibleForData = true;
    copy->additionalFields[0] = this->additionalFields[0];
    copy->additionalFields[1] = this->additionalFields[1];
    copy->header = this->header;
    if (this->length > 0) {
        uint8_t * tmp = new uint8_t [this->length];
        memcpy(tmp, this->data, this->length);
        copy->setData(tmp, this->length);
    }
    return copy;
}

uint16_t ICMPv6Message::parse(const uint8_t* buffer, uint16_t length) {
    if (length < 8) return 0;
    type = buffer[0];
    code = buffer[1];
    checksum = ((buffer[2] << 8) | buffer[3]);
    additionalFields[0] = (buffer[4] << 8) | buffer[5];
    additionalFields[1] = (buffer[6] << 8) | buffer[7];
    if (length > 8) {
        setData(&(buffer[8]), (length - 8));
    }
    return length;
}

uint16_t ICMPv6Message::writeHeaderToBuffer(uint8_t* buffer) const {
    buffer[0] = type;
    buffer[1] = code;
    buffer[2] = checksum >> 8;
    buffer[3] = checksum;
    buffer[4] = additionalFields[0] >> 8;
    buffer[5] = additionalFields[0] & 0xFF;
    buffer[6] = additionalFields[1] >> 8;
    buffer[7] = additionalFields[1] & 0xFF;
    return 8;
}

uint16_t ICMPv6Message::calcChecksum(const IPv6Address &src, const IPv6Address &dst) const {
    uint32_t chksum = 0;
    chksum = (((uint16_t) type) << 8) + code;
    chksum += additionalFields[0];
    chksum += additionalFields[1];

    return doingChecksum(chksum, data, length, src, dst,
                length + getHeaderSize(), ICMPv6Message::HeaderNumber);
}

}
