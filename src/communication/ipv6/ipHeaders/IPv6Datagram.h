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

#ifndef IPV6DATAGRAM_H_
#define IPV6DATAGRAM_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "FollowingHeader.h"
#include "IPv6Address.h"
#include "IPv6DestinationOptionsHeader.h"
#include "IPv6FragmentHeader.h"
#include "IPv6HopByHopOptionsHeader.h"
#include "IPv6RoutingHeader.h"
#include "UDPPacket.h"
#include "ICMPv6Message.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

class IPv6Datagram: public FollowingHeader {
private:
    struct fLtC_t {
        uint8_t trafficClass:  8;
        uint32_t flowLabel:    20;
        fLtC_t(): trafficClass(0), flowLabel(0) {}
    } flowL_TrafficC;

    uint8_t     hopLimit;

public:
    static const headerType_t HeaderNumber = 41;
    static const uint8_t IPV6_HEADER_SIZE = 40;

    IPv6Datagram() :
        FollowingHeader(IPv6Datagram::HeaderNumber),
        hopLimit(1),
        src(0,0,0,0),
        dst(0,0,0,0) {}
    virtual ~IPv6Datagram();

    virtual IPv6Datagram * clone() const;

    /** Allocates memory for a new header of the given type.
     * @param type type of the header (according to IANA definitions)
     * @return pointer to the newly created header
     */
    static FollowingHeader* createNextHeader(uint8_t type);

    uint16_t getFixedSize() const {
        ASSERT(false);
        return IPV6_HEADER_SIZE;
    }

    uint16_t getSize () const {
        return IPV6_HEADER_SIZE;
    }

    virtual uint16_t writeHeaderToBuffer(uint8_t* buffer) const;

    /** Parses the given data as an IPv6 header and sets the corresponding
     * values of the object.
     * @param buffer array containing the IPv6 header; must be of size length
     * @param length size of the buffer array
     * @return
     */
    uint16_t parse(const uint8_t* buffer, uint16_t length);


    inline uint32_t getFlowLabel () const { return flowL_TrafficC.flowLabel; }
    inline uint8_t getTrafficClass () const { return flowL_TrafficC.trafficClass; }
    inline uint8_t getHopLimit () const { return hopLimit; }

    /** Adds the given header to the list of headers of this datagram.
     *
     */
    void addHeader (FollowingHeader *header, int8_t pos = -1);
    inline void setFlowLabel (uint32_t flowLabel) { flowL_TrafficC.flowLabel = flowLabel; }
    inline void setTrafficClass (uint8_t trafficClass) { flowL_TrafficC.trafficClass = trafficClass; }
    inline void setHopLimit (uint8_t hopLimit) { this->hopLimit = hopLimit; }
    void decrHopLimit () {
        if (hopLimit > 0) {
            hopLimit--;
        }
    }
    IPv6Address src;
    IPv6Address dst;

	// not yet compatible with SWIG, fails parsing move assignment operator
//    IPv6Datagram & operator=(IPv6Datagram && rhs) {
//        if (this != &rhs) {
//            delete nextHeader;
//            src = rhs.src;
//            dst = rhs.dst;
//            flowL_TrafficC.flowLabel = rhs.flowL_TrafficC.flowLabel;
//            flowL_TrafficC.trafficClass = rhs.flowL_TrafficC.trafficClass;
//            hopLimit = rhs.hopLimit;
//            nextHeader = rhs.nextHeader;
//            rhs.nextHeader = NULL;
//        }
//        return *this;
//    }
private:

    /*
     * FIXME Currently just used to prevent any use of the operator, which
     * is extremely dangerous due to NOT handling the Following Headers in
     * any way. Correct would be to copy all headers and attach them to this
     * datagram -- however, another story is how deallocation of existing
     * Following Headers is handled.
     */
    IPv6Datagram & operator=(const IPv6Datagram & rhs) {
        return *this;
    }

};

}

#endif /* IPV6DATAGRAM_H_ */
