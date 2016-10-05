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

#ifndef UDPPACKET_H_
#define UDPPACKET_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "FollowingHeader.h"
#include "IPv6Address.h"

// demonstrate merging
/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

const uint16_t UDP_UNDEFINED_PORT = 0;

class UDPPacket : public FollowingHeader {
public:
    static const headerType_t HeaderNumber = 17;
    static const uint8_t UDP_HEADER_SIZE = 8;

    UDPPacket (uint16_t destPort = UDP_UNDEFINED_PORT,
               uint16_t srcPort = UDP_UNDEFINED_PORT);
    virtual ~UDPPacket ();

    virtual UDPPacket * clone() const;

    uint16_t writeHeaderToBuffer(uint8_t* buffer) const;

    uint16_t getFixedSize() const {
        return UDP_HEADER_SIZE;
    }

    virtual void setData(const uint8_t* data, uint16_t length) {
        this->data = data;
        this->length = length;
    }
    bool checkValid (const IPv6Address &src, const IPv6Address &dst) const {
        return ((checksum == 0) || (checksum == calcChecksum(src, dst)));
    }

    virtual void generateChecksum (const IPv6Address &src, const IPv6Address &dst) {
        checksum = calcChecksum(src, dst);
    }

    uint16_t parse(const uint8_t* buffer, uint16_t length);
    uint16_t parse(const uint8_t* buffer);
    uint16_t getSrcPort () const {
        return srcPort;
    }
    uint16_t getDestPort () const {
        return dstPort;
    }
    uint16_t getUpperLayerPayloadLength () const {
        return length;
    }
    uint16_t getSize () const {
        return UDP_HEADER_SIZE;
    }
    const uint8_t* getData () {
        return data;
    }
    uint16_t getChecksum () const {
        return checksum;
    }
    bool isChecksumSet () const {
        return checksum != 0;
    }
    void setSrcPort (uint16_t port) {
        srcPort = port;
    }
    void setDestPort (uint16_t port) {
        dstPort = port;
    }
    void setChecksum (uint16_t chksum) {
        checksum = chksum;
    }

private:
    uint16_t calcChecksum(const IPv6Address &src, const IPv6Address &dst) const;

    uint16_t srcPort;
    uint16_t dstPort;
    uint16_t checksum;
    uint16_t length;
    const uint8_t* data;
private:
    bool responsibleForData;
};

}

#endif /* UDPPACKET_H_ */
