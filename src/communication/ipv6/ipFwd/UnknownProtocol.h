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

#ifndef UNKNOWNPROTOCOL_H_
#define UNKNOWNPROTOCOL_H_

#include "cometos.h"
#include "ContentRequest.h"
#include "ContentResponse.h"
#include "logging.h"

namespace cometos_v6 {

class UnknownProtocol: public cometos::Module {
public:
    UnknownProtocol(const char * service_name = NULL) :
        cometos::Module(service_name),
        fromIP(this, &UnknownProtocol::handleIPRequest, "fromIP")
//        toIP(this, "toIP")
    {}

    void initialize() {}

    void handleIPRequest(ContentRequest *cRequest) {
        LOG_DEBUG("Rcvd Pckt w/ unknown Protocol from "
                << cRequest->src.getAddressPart(7));
        cRequest->response(new ContentResponse(cRequest, false));
    }

    void handleIPResponse(ContentResponse *cResponse) {
        delete cResponse;
    }

    cometos::InputGate<ContentRequest>     fromIP;
//    cometos::OutputGate<ContentRequest>    toIP;
};

}

#endif /* UNKNOWNPROTOCOL_H_ */
