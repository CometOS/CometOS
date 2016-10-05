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

#ifndef ICMPV6MESSAGE_H_
#define ICMPV6MESSAGE_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "FollowingHeader.h"
#include <cometos.h>
#include "IPv6Address.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

class ICMPv6Message : public FollowingHeader {
public:
    static const headerType_t HeaderNumber = 58;
    static const uint8_t FIXED_SIZE = 8;

    ICMPv6Message (uint8_t type = 0, uint8_t code = 0);
    virtual ~ICMPv6Message();

    virtual ICMPv6Message * clone() const;

    virtual const uint8_t* getData() { return data; }
    virtual uint16_t getUpperLayerPayloadLength() const { return length; }

    virtual uint16_t getSize() const { return FIXED_SIZE; }

    virtual uint16_t getFixedSize() const {return FIXED_SIZE;}

    virtual uint16_t writeHeaderToBuffer(uint8_t* buffer) const;
    virtual uint16_t getHeaderSize() const { return FIXED_SIZE; };
    virtual uint16_t getChecksum() const { return checksum; }
    virtual uint8_t getType() const { return type; }
    uint8_t getCode() const { return code; }
    const uint16_t* getAdditionalFields() const { return additionalFields; }

    void generateChecksum (const IPv6Address &src, const IPv6Address &dst) {
        checksum = calcChecksum(src, dst);
    }
    bool checkValid (const IPv6Address &src, const IPv6Address &dst) const {
        return (checksum == calcChecksum(src, dst));
    }
    virtual uint16_t parse(const uint8_t* buffer, uint16_t length);

    virtual void setData (const uint8_t* data, uint16_t length) {
        this->data = data;
        this->length = length;
    }
    void setChecksum(uint16_t chksum) { this->checksum = chksum; }
    void setType(uint8_t type) { this->type = type; }
    void setCode(uint8_t code) { this->code = code; }
    void setAdditionalFields(const uint16_t fields[2]) {
        additionalFields[0] = fields[0];
        additionalFields[1] = fields[1];
    }
    void setAdditionalFields(uint16_t a, uint16_t b) {
        additionalFields[0] = a;
        additionalFields[1] = b;
    }
protected:
    uint16_t calcChecksum(const IPv6Address &src, const IPv6Address &dst) const;
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    const uint8_t* data;
    uint16_t length;
    uint16_t additionalFields[2];
private:
    bool responsibleForData;
};

}

#endif /* ICMPV6MESSAGE_H_ */
