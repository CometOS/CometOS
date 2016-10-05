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

#ifndef CONTENT_REQUEST_H_
#define CONTENT_REQUEST_H_

#include <cometos.h>
#include "FollowingHeader.h"
#include "RequestResponse.h"
#include "ContentResponse.h"
#include "IPv6Address.h"
#include "LowpanBuffer.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

class ContentRequest: public cometos::Request<ContentResponse> {
public:
    IPv6Address         src;
    IPv6Address         dst;
    FollowingHeader*    content;

    ContentRequest(FollowingHeader* content,
            const IPv6Address& src,
            const IPv6Address& dst,
            const TypedDelegate<ContentResponse> &delegate =
                    TypedDelegate< ContentResponse >()) :
                        cometos::Request<ContentResponse>(delegate),
                        src(src),
                        dst(dst),
                        content(content) {}
    ContentRequest(FollowingHeader* content,
            const IPv6Address& dst,
            const TypedDelegate<ContentResponse> &delegate =
                    TypedDelegate< ContentResponse >()) :
                        cometos::Request<ContentResponse>(delegate),
                        dst(dst),
                        content(content) {}
    ContentRequest(FollowingHeader* content,
            const TypedDelegate<ContentResponse> &delegate =
                    TypedDelegate< ContentResponse >()) :
                        cometos::Request<ContentResponse>(delegate),
                        content(content) {}
    ContentRequest(const TypedDelegate<ContentResponse> &delegate =
            TypedDelegate< ContentResponse >()) :
                cometos::Request<ContentResponse>(delegate),
                content(NULL) {}


    virtual ~ContentRequest() {};
};

struct contentRequestHolder_t {
    ContentRequest      contentRequest;
    BufferInformation*  buffer;
    bool                occupied;
    contentRequestHolder_t():
        buffer(NULL), occupied(false) {}
    bool mapsTo(ContentRequest & req) {
        return &req == &contentRequest;
    }
    void freeHolder() {
        occupied = false;
        if (buffer != NULL && buffer->isUsed()) {
            buffer->free();
        }
    }
    void take(BufferInformation* data) {
        occupied = true;
        buffer = data;
    }
};

}  // namespace cometos_v6

#endif /* CONTENTREQUEST_H_ */
