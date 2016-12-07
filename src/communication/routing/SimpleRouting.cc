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
 * @author Stefan Unterschuetz
 */

#include "SimpleRouting.h"
#include "palId.h"
#include "RoutingInfo.h"
#include "logging.h"

#define ROUTING_HISTORY_REFRESH     5000
#define ROUTING_TABLE_REFRESH       30000
#define ROUTING_MAX_COSTS           20
#define ROUTING_SENDING_OFFSET      100

Define_Module(cometos::SimpleRouting);

namespace cometos {

/**This class is used for storing original request in packet*/
class RoutingRequestId: public RequestId {
public:
    RoutingRequestId(DataRequest *req = NULL) :
            req(req) {
    }
    DataRequest *req;
};

SimpleRouting::SimpleRouting(const char * name) :
        Layer(name), seq(0) {

    Tuple<int, int, int> a1(3, 4, 5);
    Tuple<int, int> a2(1, 4);
    a1.third = 2;
    a2.second = 3;

#ifdef ROUTING_ENABLE_STATS
    forwarded=0;
#endif

}

SimpleRouting::~SimpleRouting() {
}

void SimpleRouting::initialize() {
    Layer::initialize();

    // start timer for periodic updates of history, also initializes array
    schedule(new Message, &SimpleRouting::historyUpdate);

    // start timer for periodic updates of routing table, also initializes array
    schedule(new Message, &SimpleRouting::tableUpdate);
}

void SimpleRouting::handleRequest(DataRequest* msg) {

    NwkHeader nwk(msg->dst, palId_id(), seq++, 0);
    msg->getAirframe() << nwk;
    history.update(historyEntry_t(nwk.src, nwk.seq));

    node_t nextHop = getNextHop(msg->dst);
    msg->dst = nextHop;
    LOG_INFO("send to "<<nwk.dst<<" via "<<nextHop);
    if (nextHop == BROADCAST ) {
        // no response required for packets
        sendRequest(msg);
    } else {
        // sending is done with response handler
        sendRequest(
                new DataRequest(nextHop, msg->getAirframe().getCopy(),
                        createCallback(&SimpleRouting::handleResponse),
                        new RoutingRequestId(msg)));
    }

}

node_t SimpleRouting::getNextHop(node_t dst) {
    uint8_t it = table.find(dst);
    if (it == table.end()) {
        return BROADCAST ;
    } else {
        return table[it].second;
    }
}


void SimpleRouting::sendAutoReply(node_t dst) {
    if (dst == BROADCAST ) {
        return;
    }
    node_t nextHop = getNextHop(dst);
    if (nextHop == BROADCAST ) {
        return;
    }

    NwkHeader nwk;
    nwk.src = palId_id(); //getId();
    nwk.dst = dst;
    nwk.hops |= 0x40;
    Airframe* frame = new Airframe();
    (*frame) << nwk;
    DataRequest *req = new DataRequest(nextHop, frame);


#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif

    sendRequest(req);
}


void SimpleRouting::handleIndication(DataIndication* msg) {
    NwkHeader nwk;
    msg->getAirframe() >> nwk;

// duplicate filtering
    if (history.update(historyEntry_t(nwk.src, nwk.seq))) {
        delete msg;
        return;
    }

// maximal nummber of hops
    if ((nwk.hops & 0x3F) >= ROUTING_MAX_COSTS) {
        LOG_ERROR("hop limit reached, delete packet");
        delete msg;
        return;
    }

// increase hop count
    nwk.hops = (nwk.hops & 0xc0) | ((nwk.hops & 0x3F) + 1);

// update routing table
    uint8_t it = table.find(nwk.src);
    if (it != table.end()) {
        if (table[it].third >= nwk.hops) {
            table.erase(it);

            table.update(tableEntry_t(nwk.src, msg->src, nwk.hops));
            LOG_DEBUG("UPDATE OLD " << palId_id() << "-->" << nwk.src << " via "
                    << msg->src << " in " << (int) nwk.hops);

        }
    } else {
        table.update(tableEntry_t(nwk.src, msg->src, nwk.hops));
        LOG_DEBUG("UPDATE NEW " << palId_id() << "-->" << nwk.src << " via "
                << msg->src << " in " << (int) nwk.hops);
    }

// delete entry if invalid flag is set
    if (nwk.hops & 0x80) {
        table.erase(it);
    }

    if (nwk.dst == palId_id() ) {
        // packet is route reply, delete it
        if (nwk.hops & 0x40) {
            delete msg;
            return;
        }

        // sender of packet used broadcast, thus use route reply to manifest route
        if (msg->dst == BROADCAST ) {
            sendAutoReply(nwk.src);
        }

        LOG_INFO("receive from "<<nwk.src);
        msg->src = nwk.src;
        msg->dst = nwk.dst;
        msg->set(new RoutingInfo(nwk.hops & 0x3f));
        sendIndication(msg);
        return;
    }

    DataRequest *req = new DataRequest(getNextHop(nwk.dst),
            msg->getAirframe().getCopy());
    req->getAirframe() << nwk;

    // no route reply, if nextHop can not addressed directly
    if ((req->dst == BROADCAST)  && (nwk.hops & 0x40)) {
        delete msg;
        return;
    }

    LOG_DEBUG("send to "<<nwk.dst << " via " << req->dst);

// second rule means that a packet sent via BROADCAST is not allowed to become
// a unicast message

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif

    if (req->dst == BROADCAST || msg->dst == BROADCAST ) {
        req->dst = BROADCAST;
        LOG_INFO("forward to "<<nwk.dst<<" via "<<req->dst);
        sendRequest(req, intrand(ROUTING_SENDING_OFFSET));
    } else {
        LOG_INFO("forward to "<<nwk.dst<<" via "<<req->dst);
        sendRequest(
                new DataRequest(req->dst, req->getAirframe().getCopy(),
                        createCallback(&SimpleRouting::handleResponse),
                        new RoutingRequestId(req)));
    }

    if (nwk.dst == BROADCAST ) {

        msg->src = nwk.src;
        msg->dst = nwk.dst;
        msg->set(new RoutingInfo(nwk.hops & 0x3f));
        sendIndication(msg);
    } else {
        delete msg;
    }

}

void SimpleRouting::handleResponse(DataResponse* resp) {
    DataRequest *req = ((RoutingRequestId*) resp->getRequestId())->req;
    if (resp->isSuccess()) {
        req->response(new DataResponse(DataResponseStatus::SUCCESS));
        delete req;
    } else {
        NwkHeader nwk;
        req->getAirframe() >> nwk;

        table.erase(table.find(nwk.src));
        LOG_DEBUG("transmission to "<<nwk.dst<<" failed, switch to broadcast");
        nwk.hops |= 0x80;
        req->getAirframe() << nwk;
        req->dst = BROADCAST;

        // is packet is route reply, do not try to forward
        if (nwk.hops & 0x40) {
            delete req;
        } else {
            sendRequest(req);
        }
    }
    delete resp;
}

void SimpleRouting::historyUpdate(Message* timer) {
    history.refresh();
    schedule(timer, &SimpleRouting::historyUpdate, ROUTING_HISTORY_REFRESH);
}

void SimpleRouting::tableUpdate(Message* timer) {
    table.refresh();
    schedule(timer, &SimpleRouting::tableUpdate, ROUTING_TABLE_REFRESH);
}

void SimpleRouting::finish() {
#ifdef ROUTING_ENABLE_STATS
    recordScalar("forwarded", forwarded);
#endif
}

} /* namespace cometos */

bool operator==(const cometos::tableEntry_t&val1, const node_t& val2) {
    return val1.first == val2;
}
