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

#include "URL.h"

namespace cometos_v6 {

const uint8_t URL::maxNumParts;

const char* URIPart::parse(const char* url, const URIPart* prev) {
    if (url != NULL) {
        switch (*url) {
        case '/':
            type = URIPATH;
            break;
        case '&':
            if (prev == NULL || prev->getType() != URIQUERY) {
                return NULL;
            }
            /* no break */
        case '?':
            type = URIQUERY;
            break;
        }

        uint8_t pos = 1;
        while (url[pos] != '/' &&
                url[pos] != '?' &&
                url[pos] != '&' &&
                url[pos] != 0)
        {
            pos++;
            len++;
        }
        if ((url[pos] == '&' && type == URIPATH) ||
                (url[pos] == '/' && type == URIQUERY)) {
            type = NOTFOUND;
            len = 0;
        }
        pointer = url + 1;
    }
    return url + len + 1;
}

bool URL::addURIPart(const char* p, uint8_t length, URIPart::type_t t) {
    if (numParts < maxNumParts) {
        uriParts[numParts++].setPart(p, length, t);
        return true;
    }
    return false;
}

bool URL::parseURIPart(const char* p) {
    while (p != NULL && *p != 0 && numParts < maxNumParts) {
        URIPart* prev = NULL;
        if (numParts > 0) {
            prev = &(uriParts[numParts - 1]);
        }
        p = uriParts[numParts].parse(p, prev);
        numParts++;
    }
    if (p != NULL && *p == 0) {
        return true;
    }
    return false;
}

cometos::SString<255> URL::toString() const {
    cometos::SString<255> ret;

    switch (protocol) {
    case COAP:
        ret = "coap://";
        break;
    case COAPS:
        ret = "coaps://";
        break;
    default:
        ret = "unknown://";
    }

    ret += '[';
    ret += ip.str();
    ret += ']';

    if (port != 0) {
        ret += ':';
        ret.append(port);
    }

    bool query = false;

    for (uint8_t i = 0; i < numParts; i++) {
        switch (uriParts[i].getType()) {
        case URIPart::URIPATH:
            ret += '/';
            break;
        case URIPart::URIQUERY:
            if (query) {
                ret += '&';
            } else {
                ret += '?';
                query = true;
            }
            break;
        default:
            ret += "<error>";
            return ret;
        }
        ret.append(uriParts[i].getPart(), uriParts[i].getLength());
    }

    return ret;
}

#if defined(OMNETPP) || defined(BOARD_python) || defined(BOARD_local)
URL::URL(const char* url, uint16_t standartPort):
        protocol(UNKNOWN),
        port(standartPort),
        numParts(0)
{
    url = parseURLPrefix(url);

    url = parseIPv6Address(url);

    url = parsePort(url);

    while (url != NULL && *url != 0 && numParts < maxNumParts) {
        URIPart* prev = NULL;
        if (numParts > 0) {
            prev = &(uriParts[numParts - 1]);
        }
        url = uriParts[numParts].parse(url, prev);
        numParts++;
    }
}

const char* URL::parseURLPrefix(const char* url) {
    const uint8_t numCompareStrings = 2;
    const char* compareStrings[numCompareStrings] = {
      "coap://",
      "coaps://"
    };

    // get Protocol
    for (uint8_t i = 0; i < numCompareStrings; i++) {
        const char* part = beginsWith(url, compareStrings[i]);
        if (part != NULL) {
            protocol = (protocol_t)i;
            url = part;
            break;
        }
    }

    return url;
}

const char* URL::parseIPv6Address(const char* url) {
    const char* p = url;
    if (p != NULL && *p == '[') {
        char tmp[40];
        p++;
        uint8_t pos = 0;
        while (p[pos] != ']' && p[pos] != 0) {
            tmp[pos] = p[pos];
            pos++;
        }
        if (p[pos] == ']') {
            tmp[pos] = 0;
            if (ip.tryParse(tmp)) {
                p += pos + 1;
                return p;
            }
        }
    }
    return NULL;
}

const char* URL::parsePort(const char* url) {
    if (url != NULL && url[0] == ':') {
        uint8_t pos = 1;
        uint16_t prt = 0;
        while (url[pos] >= '0' && url[pos] <= '9') {
            uint16_t p = (prt * 10) + (url[pos] - '0');
            pos++;
            if (p > prt) {
                prt = p;
            } else {
                return NULL;
            }
        }
        if (url[pos] != '/') {
            return NULL;
        }
        port = prt;
        return (url + pos);
    }
    return url;
}

const char* URL::beginsWith(const char* begins, const char* with) {

    uint8_t i = 0;
    while (with[i] != 0 && begins[i] != 0 && begins[i] == with[i]) {
        i++;
    }
    if (with[i] == 0) {
        return (begins + i);
    }
    return NULL;
}
#endif

} /* namespace cometos_v6 */
