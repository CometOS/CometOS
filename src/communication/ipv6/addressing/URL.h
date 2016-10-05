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

#ifndef URL_H_
#define URL_H_

#include "IPv6Address.h"
#include "SString.h"

namespace cometos_v6 {

class URIPart {
public:
    enum type_t : uint8_t {
        NOTFOUND = 0,
        URIPATH =  1,
        URIQUERY = 2
    };

    URIPart():
        pointer(NULL),
        len(0),
        type(NOTFOUND)
    {}

    const char* parse(const char* url, const URIPart* prev = NULL);

    void setPart(const char* p, uint8_t length, type_t t) {
        pointer = p;
        len = length;
        type = t;
    }

    const char* getPart() const {
        return pointer;
    }

    uint16_t getLength() const {
        return len;
    }

    type_t getType() const {
        return type;
    }

protected:
    const char* pointer;
    uint8_t len;
    type_t type;
};

/*
 * This Class contains a URL. As the string used for the initialization is not
 * copied, it needs to be valid for as long as the URL Object is used.
 */
class URL {
public:
    static const uint8_t maxNumParts = 20;

    enum protocol_t {
        COAP = 0,
        COAPS = 1,
        UNKNOWN
    };

    URL(protocol_t protocol, const IPv6Address& ip, uint16_t port = 0):
        protocol(protocol),
        ip(ip),
        port(port),
        numParts(0)
    {}

    cometos::SString<255> toString() const;

#if defined(OMNETPP) || defined(BOARD_python) || defined(BOARD_local)
    URL(const char* url, uint16_t standartPort = 0);
#endif

    virtual ~URL() {}

    bool addURIPart(const char* p, uint8_t length, URIPart::type_t t);
    bool parseURIPart(const char* p);

    protocol_t protocol;
    IPv6Address ip;
    uint16_t port;
    URIPart uriParts[maxNumParts];
    uint8_t numParts;

protected:
#if defined(OMNETPP) || defined(BOARD_python) || defined(BOARD_local)
    const char* parseURLPrefix(const char* url);
    const char* parseIPv6Address(const char* url);
    const char* parsePort(const char* url);

    const char* beginsWith(const char* begins, const char* with);
#endif
};

} /* namespace cometos_v6 */
#endif /* URL_H_ */
