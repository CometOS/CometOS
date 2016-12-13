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

#include "SourceTreeRouting.h"
#include "RoutingInfo.h"
#include "logging.h"

Define_Module(SourceTreeRouting);

using namespace cometos;

SourceTreeRouting::SourceTreeRouting(const char *name) :
        Module(name), gateReqIn(this, &SourceTreeRouting::handleRequest,
                "gateReqIn"), gateIndOut(this, "gateIndOut"), sourceIndIn(this,
                &SourceTreeRouting::handleSourceIndication, "sourceIndIn"), sourceReqOut(
                this, "sourceReqOut"), treeIndIn(this,
                &SourceTreeRouting::handleTreeIndication, "treeIndIn"), treeReqOut(
                this, "treeReqOut"), innerIndIn(this,
                &SourceTreeRouting::handleInnerIndication, "innerIndIn"), innerReqOut(
                this, "innerReqOut"), innerReqIn(this,
                &SourceTreeRouting::handleInnerRequest, "innerReqIn"), innerIndOut(
                this, "innerIndOut") {
}

void SourceTreeRouting::initialize() {
    Module::initialize();

    isSink = par("isSink");

    schedule(new Message, &SourceTreeRouting::historyUpdate);
    seq = 0;

#ifdef ROUTING_ENABLE_STATS
    forwarded=0;
#endif

}

void SourceTreeRouting::finish() {
    cometos::Module::finish();
#ifdef ROUTING_ENABLE_STATS
    recordScalar("forwarded", forwarded);
#endif

}

void SourceTreeRouting::handleRequest(cometos::DataRequest* msg) {

    ASSERT(msg->dst!=BROADCAST);
    // currently not supported

    if (isSink) {
        if (paths.count(msg->dst) == 0) {
            msg->response(new DataResponse(DataResponseStatus::NO_ROUTE));
            delete msg;
            return;
        }LOG_INFO("send source message to "<<msg->dst);

        NwkHeader nwk = NwkHeader(msg->dst, getId(), seq++, 0);
        path_t path = paths[msg->dst];
        ASSERT(path.getSize()>0);
        msg->dst = path.popBack();
        msg->getAirframe() << nwk << path;
        history.update(historyEntry_t(nwk.src, nwk.seq));

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif

        sourceReqOut.send(msg);
    } else {
        LOG_INFO("send message via tree routing");
        msg->getAirframe().set(new PathObj);
        innerReqOut.send(msg);
    }

}

void SourceTreeRouting::handleInnerRequest(cometos::DataRequest* msg) {
    ///  currently not supported
    if (msg->dst == BROADCAST ) {
        LOG_WARN("roouting message failed");
        msg->response(new DataResponse(DataResponseStatus::NO_ROUTE));
        delete msg;
        return;
    }

    ASSERT(msg->dst!=BROADCAST);
    PathObj* p = msg->getAirframe().get<PathObj>();
    ASSERT(p);LOG_INFO("add path to inner request and forward to "<<msg->dst);

    if (p->path.getSize() == p->path.getMaxSize()) {
        LOG_WARN(" path length insufficient");
        msg->response(new DataResponse(DataResponseStatus::NO_ROUTE));
        delete msg;
        return;
    }

    p->path.pushBack(getId());
    msg->getAirframe() << p->path;
    treeReqOut.send(msg);
}

void SourceTreeRouting::handleInnerIndication(cometos::DataIndication* msg) {

    PathObj* p = msg->getAirframe().get<PathObj>();
    ASSERT(p);ASSERT(isSink);LOG_INFO("receive tree message");
    //for (int i=0;i<p->path.getSize();i++) {
    //	cout<<" "<<p->path[i]<<endl;
    //}
    // store path to source of packet
    paths[msg->src] = p->path;
    gateIndOut.send(msg);
}

void SourceTreeRouting::handleSourceIndication(cometos::DataIndication* msg) {
    path_t path;
    NwkHeader nwk;
    msg->getAirframe() >> path >> nwk;

    // duplicate filtering
    if (history.update(historyEntry_t(nwk.src, nwk.seq))) {
        delete msg;
        return;
    }

    // increase hop count
    nwk.hops++;

    if (nwk.dst == getId()) {
        LOG_INFO("receive source message");
        msg->src = nwk.src;
        msg->dst = nwk.dst;
        msg->set(new RoutingInfo(nwk.hops));
        gateIndOut.send(msg);
        return;
    }

    ASSERT(path.getSize()>0);

    node_t next = path.popBack();
    LOG_INFO("forward message to "<<nwk.dst<< " via "<<next);
    msg->getAirframe() << nwk << path;
#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif
    sourceReqOut.send(new DataRequest(next, msg->decapsulateAirframe()));
    delete msg;
}

void SourceTreeRouting::handleTreeIndication(cometos::DataIndication* msg) {

    PathObj* p = new PathObj;
    msg->getAirframe() >> p->path;
    LOG_INFO("receive tree message");

    msg->getAirframe().set(p);
    innerIndOut.send(msg);
}

void SourceTreeRouting::historyUpdate(Message* timer) {
    history.refresh();
    schedule(timer, &SourceTreeRouting::historyUpdate, ROUTING_HISTORY_REFRESH);
}

