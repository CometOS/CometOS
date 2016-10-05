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

#ifndef IPV6REQUEST_H_
#define IPV6REQUEST_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "IPv6Datagram.h"
#include "DummyRequest.h"
#include "RequestResponse.h"
#include "IPv6Response.h"
#include "Ieee802154MacAddress.h"


/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {


// TODO split IPv6Request and IPv6Indication to make usage clearer



class IPv6RequestInformation {
public:
    IPv6RequestInformation(IPv6Datagram* datagram = NULL,
            const Ieee802154MacAddress& dstMacAddress = Ieee802154MacAddress(),
            const Ieee802154MacAddress& srcMacAddress = Ieee802154MacAddress()):
                dstMacAddress(dstMacAddress),
                srcMacAddress(srcMacAddress),
                datagram(datagram)
            {}

    Ieee802154MacAddress    dstMacAddress;
    Ieee802154MacAddress    srcMacAddress;
    IPv6Datagram*           datagram;
};


class IPv6Request: public cometos::Request<IPv6Response> {
    public:
        IPv6RequestInformation  data;

        IPv6Request(const TypedDelegate<IPv6Response> &delegate = TypedDelegate
                        < IPv6Response >()) :
                            cometos::Request<IPv6Response>(delegate) {}
        IPv6Request(uint16_t src[4], uint16_t dst[4], IPv6Datagram* datagram,
                const TypedDelegate<IPv6Response> &delegate = TypedDelegate
                        < IPv6Response >()) :
                            cometos::Request<IPv6Response>(delegate),
                            data(datagram, dst, src) {}
        IPv6Request(const Ieee802154MacAddress& src,
                const Ieee802154MacAddress& dst, IPv6Datagram* datagram,
                const TypedDelegate<IPv6Response> &delegate = TypedDelegate
                        < IPv6Response >()) :
                            cometos::Request<IPv6Response>(delegate),
                            data(datagram, dst, src) {}
        IPv6Request(IPv6Datagram* datagram,
                const TypedDelegate<IPv6Response> &delegate = TypedDelegate
                        < IPv6Response >()) :
                            cometos::Request<IPv6Response>(delegate),
                            data(datagram) {}

        virtual ~IPv6Request() {};

    };


}  // namespace cometos_v6

#endif /* IPV6REQUEST_H_ */
