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


#ifndef IPV6FRAGMENTHEADER_H_
#define IPV6FRAGMENTHEADER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "FollowingHeader.h"
#include "IPv6ExtensionHeaderOption.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

class IPv6FragmentHeader : public FollowingHeader {
public:
    static const headerType_t HeaderNumber = 44;
    static const uint8_t FIXED_SIZE = 8;

    IPv6FragmentHeader();
    IPv6FragmentHeader(uint16_t offset, bool mFlag, uint32_t identification);
    ~IPv6FragmentHeader();

    virtual IPv6FragmentHeader * clone() const;

    uint16_t getFragmentOffset () const;
    virtual const uint8_t* getData() { return NULL; }
    uint16_t getUpperLayerPayloadLength() const;
    uint16_t writeHeaderToBuffer(uint8_t* buffer) const;

    uint16_t getFixedSize() const {
        return getSize();
    }

    uint16_t getSize() const {
        return FIXED_SIZE;
    }

    bool getMFlag ();
    uint32_t getIdentification () const;
    void setFragmentOffset (uint16_t offset);
    void setMFlag ();
    void unsetMFlag ();
    void setIdentification (uint32_t identification);
    uint16_t parse (const uint8_t* buffer, uint16_t length);
protected:
    uint16_t fragmentOffset;
    bool mFlag;
    uint32_t identification;
};

inline uint16_t IPv6FragmentHeader::getUpperLayerPayloadLength () const {
    return 0;
}

inline void IPv6FragmentHeader::setFragmentOffset (uint16_t offset) {
    fragmentOffset = offset;
}


inline void IPv6FragmentHeader::setIdentification (uint32_t identification) {
    this->identification = identification;
}


inline void IPv6FragmentHeader::setMFlag() {
    mFlag = true;
}

inline void IPv6FragmentHeader::unsetMFlag() {
    mFlag = false;
}

inline uint16_t IPv6FragmentHeader::getFragmentOffset () const {
    return fragmentOffset;
}

inline bool IPv6FragmentHeader::getMFlag () {
    return mFlag;
}

inline uint32_t IPv6FragmentHeader::getIdentification () const {
    return identification;
}

}

#endif /* IPV6FRAGMENTHEADER_H_ */
