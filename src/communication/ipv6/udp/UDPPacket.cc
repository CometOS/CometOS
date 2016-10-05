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
 * @author Martin Ringwelski
 */

#include "UDPPacket.h"
#include "logging.h"

namespace cometos_v6 {

const headerType_t UDPPacket::HeaderNumber;
const uint8_t UDPPacket::UDP_HEADER_SIZE;

UDPPacket::UDPPacket (uint16_t destPort,
               uint16_t srcPort) :
                  FollowingHeader(UDPPacket::HeaderNumber),
                  srcPort(srcPort),
                  dstPort(destPort),
                  checksum(0),
                  length(0),
                  data(NULL),
                  responsibleForData(false)
{}

UDPPacket::~UDPPacket() {
    if (responsibleForData) {
        delete [] data;
    }
}

UDPPacket * UDPPacket::clone() const {
    UDPPacket * copy = new UDPPacket(this->dstPort, this->srcPort);
    copy->checksum = this->checksum;
    copy->responsibleForData = true;
    copy->header = this->header;
    copy->nextHeader = NULL;
    if (this->length > 0) {
        uint8_t * tmp = new uint8_t[this->length];
        memcpy(tmp, this->data, this->length);
        copy->setData(tmp, this->length);
    }
    return copy;
}

uint16_t UDPPacket::parse(const uint8_t* buffer) {
    length = ((((uint16_t) buffer[4]) << 8) | buffer[5]);
    return parse(buffer, length);
}


uint16_t UDPPacket::parse(const uint8_t* buffer, uint16_t length) {
    srcPort = ((buffer[0] << 8) | buffer[1]);
    dstPort = ((buffer[2] << 8) | buffer[3]);
    if (((((uint16_t)buffer[4]) << 8) | buffer[5]) != length) {
        return 0;
    }
    checksum = ((buffer[6] << 8) | buffer[7]);
    setData(&(buffer[UDP_HEADER_SIZE]), length-UDP_HEADER_SIZE);
    return length;
}

uint16_t UDPPacket::calcChecksum(const IPv6Address &src, const IPv6Address &dst) const {
    uint32_t chksum = 0;
    chksum = srcPort;
    chksum += dstPort;
    chksum += getSize() + getUpperLayerPayloadLength();
    return doingChecksum(chksum, data, length, src, dst,
            getSize(), UDPPacket::HeaderNumber);
}

uint16_t UDPPacket::writeHeaderToBuffer(uint8_t* buffer) const {
    buffer[0] = srcPort >> 8;
    buffer[1] = srcPort;
    buffer[2] = dstPort >> 8;
    buffer[3] = dstPort;
    uint16_t payloadSize = getSize() + getUpperLayerPayloadLength();
    buffer[4] = payloadSize >> 8;
    buffer[5] = payloadSize;
    buffer[6] = checksum >> 8;
    buffer[7] = checksum;
    return 8;
}

}
