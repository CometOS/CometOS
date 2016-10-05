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

#include "IPv6RoutingHeader.h"

namespace cometos_v6 {

const headerType_t IPv6RoutingHeader::HeaderNumber;

IPv6RoutingHeader::IPv6RoutingHeader() :
    FollowingHeader(IPv6RoutingHeader::HeaderNumber),
    length(0),
    routingType(0),
    segmentsLeft(0),
    data(NULL)
{}

IPv6RoutingHeader::IPv6RoutingHeader(uint8_t routingType, uint8_t segmentsLeft,
        uint8_t* data, uint8_t length) :
    FollowingHeader(IPv6RoutingHeader::HeaderNumber),
    length(length),
    routingType(routingType),
    segmentsLeft(segmentsLeft),
    data(data)
{}

IPv6RoutingHeader::~IPv6RoutingHeader() {
}

IPv6RoutingHeader * IPv6RoutingHeader::clone () const {
    ASSERT(false);
    return new IPv6RoutingHeader();
}

uint16_t IPv6RoutingHeader::writeHeaderToBuffer(uint8_t* buffer) const {
    uint16_t p = 0;
    if (nextHeader != NULL) {
        buffer[p++] = nextHeader->getHeaderType();
    } else {
        buffer[p++] = FollowingHeader::NoNextHeaderNumber;
    }
    buffer[p++] = length>>3;
    buffer[p++] = routingType;
    buffer[p++] = segmentsLeft;
    memcpy(&(buffer[p]), data, length);
    p += length;
    return p;
}

uint16_t IPv6RoutingHeader::parse (const uint8_t* buffer, uint16_t length) {
    uint16_t p = 0;
    p++; //uint8_t type = buffer[p++];
    this->length = buffer[p++] << 3;
    routingType = buffer[p++];
    segmentsLeft = buffer[p++];
    this->data = &(buffer[p]);
    // TODO Options are not recognized
    return p + (this->length);
}

}
