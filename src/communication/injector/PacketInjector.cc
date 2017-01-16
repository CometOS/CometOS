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
 * @author Andreas Weigel
 */

#include "PacketInjector.h"

#include "OutputStream.h"

namespace cometos {

PacketInjector::PacketInjector(const char * name) :
    LowerEndpoint(name)
{}


void PacketInjector::initialize() {
    LowerEndpoint::initialize();

}

void PacketInjector::handleRequest(DataRequest * req) {
    delete(req);
//    getCout() << "PI_recv: ";
//    for (int i=0; i<req->getAirframe().getLength(); i++) {
//        getCout() << req->getAirframe().getArray().getBuffer()[i];
//    }
//    getCout() << endl;
}

void PacketInjector::sendPkt(const uint8_t * data, pktSize_t len, node_t src, node_t dst) {
    if (data != NULL) {
   // if (data != NULL && len >= 0) { len >= 0 is always true
        if (len <= AIRFRAME_MAX_SIZE) {
            AirframePtr pkt = make_checked<Airframe>();
//            getCout() << "PI_send: ";
            for (pktSize_t i=0; i < len; i++) {
                *pkt << data[i];
//                getCout() << data[i];
            }
//            getCout() << endl;
            sendIndication(new DataIndication(pkt,src, dst));
            return;
        }
    }
    ASSERT(false);
}

} // namespace cometos
