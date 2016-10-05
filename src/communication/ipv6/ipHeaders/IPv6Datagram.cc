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

#include "IPv6Datagram.h"
#include "logging.h"

namespace cometos_v6 {

const headerType_t IPv6Datagram::HeaderNumber;
const uint8_t IPv6Datagram::IPV6_HEADER_SIZE;

IPv6Datagram::~IPv6Datagram() {
}

IPv6Datagram * IPv6Datagram::clone() const {
    IPv6Datagram * copy = new IPv6Datagram();
    copy->src = this->src;
    copy->dst = this->dst;
    copy->flowL_TrafficC = this->flowL_TrafficC;
    copy->header = this->header;
    copy->hopLimit = this->hopLimit;
    copy->nextHeader = NULL;
    return copy;
}

uint16_t IPv6Datagram::writeHeaderToBuffer(uint8_t* buffer) const {
    uint16_t p = IPV6_HEADER_SIZE;
    buffer[0] = 0x60 | (flowL_TrafficC.trafficClass>>4);
    buffer[1] = (flowL_TrafficC.trafficClass<<4) | ((flowL_TrafficC.flowLabel>>16)&0xF);
    buffer[2] = (flowL_TrafficC.flowLabel>>8) & 0xFF;
    buffer[3] = flowL_TrafficC.flowLabel & 0xFF;
    uint16_t payloadSize = getSize() - IPV6_HEADER_SIZE;
    buffer[4] = (uint8_t)(payloadSize >> 8);
    buffer[5] = (uint8_t)(payloadSize & 0xFF);
    buffer[7] = hopLimit;
    src.writeAddress(&(buffer[8]));
    dst.writeAddress(&(buffer[24]));
    if (nextHeader != NULL) {
        buffer[6] = nextHeader->getHeaderType();
        p += nextHeader->writeHeaderToBuffer((&(buffer[IPV6_HEADER_SIZE])));
    } else {
        buffer[6] = FollowingHeader::NoNextHeaderNumber;
    }
    return p;
}

void IPv6Datagram::addHeader (FollowingHeader *header, int8_t pos) {
    ASSERT(header != NULL);
    FollowingHeader* tmp = nextHeader;
    if (pos == -1) {
        nextHeader = header;
        nextHeader->setNextHeader(tmp);
    } else {
        for (; (pos > 0) && (tmp->getNextHeader() != NULL); pos--) {
            tmp = tmp->getNextHeader();
        }
        header->setNextHeader(tmp->getNextHeader());
        tmp->setNextHeader(header);
    }
}

FollowingHeader* IPv6Datagram::createNextHeader(uint8_t type) {
    FollowingHeader* ret = NULL;
    switch (type) {
        case ICMPv6Message::HeaderNumber:
            ret = new ICMPv6Message();
            break;
        case UDPPacket::HeaderNumber:
            ret = new UDPPacket();
            break;
        case IPv6DestinationOptionsHeader::HeaderNumber:
            ret = new IPv6DestinationOptionsHeader();
            break;
        case IPv6FragmentHeader::HeaderNumber:
            ret = new IPv6FragmentHeader();
            break;
        case IPv6HopByHopOptionsHeader::HeaderNumber:
            ret = new IPv6HopByHopOptionsHeader();
            break;
        case IPv6RoutingHeader::HeaderNumber:
            ret = new IPv6RoutingHeader();
            break;
        case IPv6Datagram::NoNextHeaderNumber:
            ret = NULL;
            break;
        default:
            // unsupported header format
            break;
    }
    return ret;
}

uint16_t IPv6Datagram::parse(const uint8_t* buffer, uint16_t length) {
        uint16_t ret = 0;
        if (length > 39) {
            flowL_TrafficC.trafficClass = (buffer[ret] << 4) |
                    (buffer[ret + 1] >> 4);
            ret++;
            flowL_TrafficC.flowLabel = (((uint32_t) buffer[ret] & 0x0F) << 16)
                    | ((uint16_t) buffer[ret + 1] << 8) | buffer[ret + 2];
            ret += 3;
            uint16_t pLen = ((uint16_t)(buffer[ret]) << 8) | (buffer[ret+1]);
            if ((pLen + 40) != length) return 0;
            ret += 2;
            FollowingHeader* nxt = createNextHeader(buffer[ret]);
            LOG_DEBUG("Created next header of type " << (int)buffer[ret] << "; addr=" << (uintptr_t) nxt);
            setNextHeader(nxt);
            ret++;
            setHopLimit(buffer[ret]);
            ret++;
            src = &(buffer[ret]);
            ret += 16;
            dst = &(buffer[ret]);
            ret += 16;
            if (ret < length) {
                if (nxt == NULL) return 0;
                while ( ret < length &&
                        nxt->getHeaderType() != UDPPacket::HeaderNumber &&
                        nxt->getHeaderType() != ICMPv6Message::HeaderNumber) {
                    FollowingHeader* tnxt = createNextHeader(buffer[ret]);
                    LOG_DEBUG("Created next header of type " << (int)buffer[ret] << "; addr=" << (uintptr_t) tnxt);
                    uint16_t t = nxt->parse(&(buffer[ret]), length - ret);
                    if (t == 0 || tnxt == NULL) return 0;
                    ret += t;
                    nxt->setNextHeader(tnxt);
                    nxt = tnxt;
                }
                uint16_t t = nxt->parse(&(buffer[ret]), length - ret);
                if (t == 0) return 0;
                ret += t;
            }
        }
        return ret;
    }

} // namespace
