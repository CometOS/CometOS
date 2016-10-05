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

#ifndef IPV6ROUTINGHEADER_H_
#define IPV6ROUTINGHEADER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "FollowingHeader.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

class IPv6RoutingHeader : public FollowingHeader {
public:
    static const headerType_t HeaderNumber = 43;
    static const uint8_t FIXED_SIZE = 4;

    IPv6RoutingHeader ();
    IPv6RoutingHeader (uint8_t routingType, uint8_t segmentsLeft,
            uint8_t* data, uint8_t length);
    ~IPv6RoutingHeader();

    // TODO: implement
    virtual IPv6RoutingHeader * clone() const;

    uint8_t getLength () const;
    uint8_t getRoutingType () const {
        return routingType;
    }
    uint8_t getSegmentsLeft () const {
        return segmentsLeft;
    }
    uint16_t writeHeaderToBuffer(uint8_t* buffer) const;

    const uint8_t* getHData() const { return data; }

    inline uint16_t getHDataLength () const {return length;}
    uint16_t getSize () const {
        return length+FIXED_SIZE;
    }

    uint16_t getFixedSize() const {
        return FIXED_SIZE;
    }

    void setRoutingType (uint8_t type) {
        routingType = type;
    }
    void setSegmentsLeft (uint8_t segmentsLeft) {
        this->segmentsLeft = segmentsLeft;
    }
    void decrSegmentsLeft () {
        this->segmentsLeft = segmentsLeft-1;
    }
    virtual void setData (const uint8_t* data,
                  uint16_t length)  {
        this->length = length;
        this->data = data;
    }
    bool dataAdded;
    uint16_t parse (const uint8_t* buffer, uint16_t length);
protected:
    uint8_t length;
    uint8_t routingType;
    uint8_t segmentsLeft;
    const uint8_t* data;
};

}

#endif /* IPV6ROUTINGHEADER_H_ */
