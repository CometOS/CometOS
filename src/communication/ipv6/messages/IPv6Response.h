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

#ifndef IPV6_RESPONSE_H_
#define IPV6_RESPONSE_H_

#include "RequestResponse.h"

namespace cometos_v6 {

class IPv6Request;

class IPv6Response: public cometos::Response {
public:
    /** used to report fail reasons back to lowpan layer
     *  TODO we possibly should get the handling of error
     *  codes into the IPLayer instead of letting it report widly all its
     *  errors somewhere else */
    enum IPv6ResponseCode {
        IPV6_RC_SUCCESS = 0,
        IPV6_RC_OUT_OF_UPPER_MSGS = 1,
        IPV6_RC_OUT_OF_LOWER_MSGS = 2,
        IPV6_RC_NO_ROUTE = 3,
        IPV6_RC_QUEUE_ABORT = 4,

        /** indicates that this failure status was already counted elsewhere
         *  TODO workaround for correctly using LowpanDataIndication/Response
         *  pairs and putting handling of result codes in the correct place...*/
        IPV6_RC_FAIL_ALREADY_COUNTED = 5,
        IPV6_RC_OUT_OF_HOPS = 6,
        IPV6_RC_FAIL_FROM_UPPER = 7
    };
    typedef uint8_t ipv6ResponseCode_t;

    IPv6Response(IPv6Request* refers, ipv6ResponseCode_t success = IPV6_RC_FAIL_FROM_UPPER, bool freeBuf = true):
        success(success),
        freeBuf(freeBuf),
        reserved(0),
        refersTo(refers) {}
    uint8_t         success:4;
    uint8_t         freeBuf:1;
    uint8_t         reserved:3;
    IPv6Request*    refersTo;
};

} // namespace cometos_v6


#endif
