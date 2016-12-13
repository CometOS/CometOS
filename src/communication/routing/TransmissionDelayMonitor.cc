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

#include "TransmissionDelayMonitor.h"
#include "RoutingInfo.h"
#include "Object.h"

Define_Module(cometos::TransmissionDelayMonitor);

using omnetpp::simtime_t;

namespace cometos {

class PacketReceiveInfo: public Object {
public:

    PacketReceiveInfo(simtime_t time) :
            time(time) {
    }

    virtual Object* getCopy() const {
        return new PacketReceiveInfo(time);
    }

    simtime_t time;
};

class RoutingRequestId: public RequestId {
public:
    RoutingRequestId(DataRequest *req, simtime_t start) :
            req(req), start(start) {
    }
    DataRequest *req;
    simtime_t start;
};

void TransmissionDelayMonitor::finish() {
    Layer::finish();
    if (statsDelay.getCount() > 0) {
        recordScalar("txTime",statsDelay.getMean());
    }
    if (statsWaiting.getCount() > 0) {
        recordScalar("txWaitingTime",statsWaiting.getMean());
    }

}

void TransmissionDelayMonitor::initialize() {
    Layer::initialize();
}

void TransmissionDelayMonitor::handleRequest(DataRequest* req) {

    if (req->getAirframe().has<PacketReceiveInfo>()) {
        simtime_t waiting = omnetpp::simTime() - req->getAirframe().get<PacketReceiveInfo>()->time;
        statsWaiting.collect(waiting);
    }

    DataRequest *nreq = new DataRequest(req->dst, req->decapsulateAirframe(),
            createCallback(&TransmissionDelayMonitor::handleResponse),
            new RoutingRequestId(req, omnetpp::simTime()));

    *((ObjectContainer*) nreq) = *((ObjectContainer*) req);

    sendRequest(nreq);

}

void TransmissionDelayMonitor::handleIndication(DataIndication* msg) {
    msg->getAirframe().set(new PacketReceiveInfo(omnetpp::simTime()));
    sendIndication(msg);
}

void TransmissionDelayMonitor::handleResponse(DataResponse* resp) {
    DataRequest *req = ((RoutingRequestId*) resp->getRequestId())->req;

    simtime_t delay = omnetpp::simTime()
            - ((RoutingRequestId*) resp->getRequestId())->start;

    statsDelay.collect(delay);

    ASSERT(req!=NULL);
    req->response(new DataResponse(resp->status));
    delete req;
    delete resp;
}

} /* namespace cometos */
