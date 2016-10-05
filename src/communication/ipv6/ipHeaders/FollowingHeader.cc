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

#include "FollowingHeader.h"

namespace cometos_v6 {

const headerType_t FollowingHeader::NoNextHeaderNumber;
const uint8_t FollowingHeader::IPV6_EXTENSION_HEADER_ALIGNMENT_SIZE;

FollowingHeader::FollowingHeader(uint8_t headerType, FollowingHeader* next) :
            header(headerType),
            nextHeader(next)
{}

FollowingHeader::~FollowingHeader() {
    while (nextHeader != NULL) {
        removeHeader(0);
    }
};

uint16_t doingChecksum(uint32_t chksum, const uint8_t* data, uint16_t length, const IPv6Address &src, const IPv6Address &dst, uint16_t allLength, uint8_t headerNumber) {
    for (uint16_t i = 0; i < length; i++) {
        chksum += ((uint16_t) data[i++]) << 8;
        if (i < length) {
            chksum += data[i];
        }
    }
    for (uint8_t i = 0; i < 8; i++) {
        chksum += src.getAddressPart(i) + dst.getAddressPart(i);
    }
    chksum += allLength + headerNumber;
    while (chksum > 0xFFFF) {
        chksum = (chksum & 0xFFFF) + (chksum >> 16);
    }
    if (chksum != 0xFFFF) {
        return ~chksum;
    } else {
        return 0xFFFF;
    }
}

FollowingHeader* FollowingHeader::cloneAll() const {
    FollowingHeader * newHeader;
    newHeader = this->clone();
    FollowingHeader * curr = newHeader;
    for (uint8_t i = 0; i < getNumNextHeaders(); i++) {
        curr->setNextHeader(getNextHeader(i)->clone());
        curr = curr->getNextHeader(0);
    }
    return newHeader;
}

void FollowingHeader::setNextHeader(FollowingHeader* next) {
    nextHeader = next;
}

FollowingHeader* FollowingHeader::decapsulateAllHeaders() {
    FollowingHeader * ret = nextHeader;
    nextHeader = NULL;
    return ret;
}

headerType_t FollowingHeader::getHeaderType () const {
    return header;
}

FollowingHeader* FollowingHeader::getNextHeader(uint8_t num) const {
   if (num == 0) {
       return nextHeader;
   } else if (nextHeader != NULL) {
       return nextHeader->getNextHeader(num - 1);
   }
   return NULL;
}


FollowingHeader* FollowingHeader::getLastHeader() {
    if (nextHeader == NULL) {
        return this;
    } else {
        return nextHeader->getLastHeader();
    }
}

uint8_t FollowingHeader::getNumNextHeaders() const {
    if (nextHeader == NULL) {
        return 0;
    } else {
        return (1 + nextHeader->getNumNextHeaders());
    }
}

void FollowingHeader::removeHeader(int8_t number, bool delHeader) {
    if (number == 0) {
        if (nextHeader != NULL) {
            FollowingHeader* tmp = nextHeader->getNextHeader();
            // only remove the first header!
            nextHeader->nextHeader = NULL;
            if (delHeader) delete nextHeader;
            nextHeader = tmp;
        }
    } else if (number == -1 && nextHeader != NULL) {
        removeHeader(getNumNextHeaders() - 1, delHeader);
    } else if (nextHeader != NULL) {
        nextHeader->removeHeader(number - 1, delHeader);
    }
}

FollowingHeader* FollowingHeader::decapsulateHeader(int8_t number) {
    FollowingHeader* ret = NULL;
    if (number == 0) {
        ret = nextHeader;
        if (nextHeader != NULL) {
            FollowingHeader* tmp = nextHeader->getNextHeader();
            nextHeader = tmp;
        }
    } else if (number == -1 && nextHeader != NULL) {
        ret = decapsulateHeader(getNumNextHeaders() - 1);
    } else if (nextHeader != NULL) {
        ret = nextHeader->decapsulateHeader(number - 1);
    }
    return ret;
}

uint16_t FollowingHeader::getAlignedSize() {
    uint8_t fillAlign = IPV6_EXTENSION_HEADER_ALIGNMENT_SIZE - 1;
    uint8_t alignMask = ~(IPV6_EXTENSION_HEADER_ALIGNMENT_SIZE - 1);
    return (getSize() + fillAlign) & alignMask;
}

uint16_t FollowingHeader::getCompleteHeaderLength() const {
    uint16_t ret = getSize();
    if (nextHeader != NULL) {
        return ret + nextHeader->getCompleteHeaderLength();
    } else {
        return getSize();
    }
}

const uint8_t* FollowingHeader::getData() {
    if (nextHeader != NULL) {
        return nextHeader->getData();
    } else {
    }
        return NULL;
}

void FollowingHeader::setData(const uint8_t* data, uint16_t length)
{}


void FollowingHeader::generateChecksum(const IPv6Address& src, const IPv6Address& dst)
{}


uint16_t FollowingHeader::getUpperLayerPayloadLength() const {
    if (nextHeader != NULL) {
        return nextHeader->getUpperLayerPayloadLength();
    } else {
        return 0;
    }
}

}
