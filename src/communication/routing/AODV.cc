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

#include "AODV.h"
#include "palId.h"
#include "RoutingInfo.h"
#include "SimpleRouting.h"
#include "logging.h"

#include "MacAbstractionBase.h"

Define_Module(cometos::AODV);

#define LQI_FILTER 210

namespace cometos {

/**This class is used for storing original request in packet*/
class RoutingRequestId: public RequestId {
public:
    RoutingRequestId(DataRequest *req = NULL) :
            req(req) {
    }
    DataRequest *req;
};

AODV::AODV(const char * name) :
        Layer(name), rreqIndIn(this, &AODV::handleRreqIndication, "rreqIndIn"), rreqReqOut(
                this, "rreqReqOut"), rrepIndIn(this,
                &AODV::handleRrepIndication, "rrepIndIn"), rrepReqOut(this,
                "rrepReqOut"), rerrIndIn(this, &AODV::handleRerrIndication,
                "rerrIndIn"), rerrReqOut(this, "rerrReqOut"), seq(0) {

#ifdef ROUTING_ENABLE_STATS
    forwarded=0;
    control=0;
    queueOverflow=0;
#endif

}

AODV::~AODV() {
}

void AODV::initialize() {
    Layer::initialize();

    // start timer for periodic updates of history, also initializes array
    schedule(new Message, &AODV::historyUpdate);

    // start timer for periodic updates of routing table, also initializes array
    schedule(new Message, &AODV::tableUpdate);

    sendingOffset = par("sendingOffset");
}

void AODV::handleTimeout(DataRequest* msg) {

    LOG_INFO("delete message after timeout");

    queue.erase(queue.find(msg));
    msg->response(new DataResponse(DataResponseStatus::EXPIRED));
    delete msg;
}

void AODV::handleRequest(DataRequest* msg) {

    NwkHeader nwk(msg->dst, palId_id(), seq++, 0);
    msg->getAirframe() << nwk;
    history.update(historyEntry_t(nwk.src, nwk.seq));

    forwardRequest(msg);

}

void AODV::forwardRequest(DataRequest* msg) {

    // check if queue is empty
    if (queue.full()) {
#ifdef ROUTING_ENABLE_STATS
        queueOverflow++;
#endif
        msg->response(new DataResponse(DataResponseStatus::QUEUE_FULL));
        delete msg;
        return;
    }

    node_t nextHop = getNextHop(msg->dst);

    if (nextHop == BROADCAST && msg->dst != BROADCAST ) {
        LOG_INFO("send rreq to "<<msg->dst);
        //request can currently not processed add packet to queue
        queue.push_back(msg);
        schedule(msg, &AODV::handleTimeout, ROUTING_TIMEOUT);
        //INITIATE DISCOVERY
        // send route reply
        DataRequest *req = new DataRequest(BROADCAST, new Airframe);
        NwkHeader nwk(msg->dst, getId(), seq++, 0);
        req->getAirframe() << nwk;

#ifdef ROUTING_ENABLE_STATS
        control++;
#endif
        rreqReqOut.send(req, intrand(sendingOffset));
    } else {
        LOG_INFO("send message to "<<msg->dst << " via "<<nextHop);
        msg->dst = nextHop;
        ASSERT(!isScheduled(msg));

        uint16_t offset = 0;
        if (nextHop == BROADCAST ) {
            offset = intrand(sendingOffset);
        }

        sendRequest(
                new DataRequest(nextHop, msg->getAirframe().getCopy(),
                        createCallback(&AODV::handleResponse),
                        new RoutingRequestId(msg)), offset);
    }

}

node_t AODV::getNextHop(node_t dst) {
    uint8_t it = table.find(dst);
    if (it == table.end()) {
        return BROADCAST ;
    } else {
        return table[it].second;
    }
}

bool AODV::checkHeader(NwkHeader &nwk) {
    ASSERT(nwk.dst != BROADCAST);

    // duplicate filtering
    if (history.update(historyEntry_t(nwk.src, nwk.seq))) {
        return false;
    }

    // maximal nummber of hops
    if (nwk.hops >= ROUTING_MAX_COSTS) {
        LOG_ERROR("hop limit reached, delete packet");
        return false;
    }

    // increase hop count
    nwk.hops++;
    return true;
}

void AODV::handleRreqIndication(DataIndication* msg) {

    MacRxInfo* phy = msg->get<MacRxInfo>();
    if (phy->lqi < LQI_FILTER) {
        LOG_DEBUG("Below LQI Filter: " << (int)phy->lqi);
        delete msg;
        return;
    }

    NwkHeader nwk;
    msg->getAirframe() >> nwk;

    if (!checkHeader(nwk)) {
        delete msg;
        return;
    }

    updateRoutingTable(msg->src, nwk.src, nwk.hops);

    if (nwk.dst == palId_id()) {
        node_t nextHop = getNextHop(nwk.src);
        if (nextHop != BROADCAST ) {

            DataRequest *req = new DataRequest(nextHop, new Airframe);
            nwk.dst = nwk.src;
            ASSERT(nwk.dst!=BROADCAST);
            nwk.src = getId();
            nwk.hops = 0;
            nwk.seq = seq++;
            req->getAirframe() << nwk;
#ifdef ROUTING_ENABLE_STATS
            control++;
#endif
            // send route reply
            LOG_INFO("send rrep");
            rrepReqOut.send(req);
        }

    } else {
        LOG_INFO("forward rreq to "<<nwk.dst);
        // forward route request
        DataRequest *req = new DataRequest(BROADCAST,
                msg->decapsulateAirframe());
        req->getAirframe() << nwk;
#ifdef ROUTING_ENABLE_STATS
        control++;
#endif
        rreqReqOut.send(req, intrand(sendingOffset));
    }

    delete msg;
}

void AODV::handleRrepIndication(DataIndication* msg) {
    NwkHeader nwk;
    msg->getAirframe() >> nwk;

    if (!checkHeader(nwk)) {
        delete msg;
        return;
    }

    updateRoutingTable(msg->src, nwk.src, nwk.hops);

    if (nwk.dst == palId_id()) {
        LOG_INFO("received rrep from "<<nwk.src);ASSERT(getNextHop(nwk.src)!=BROADCAST);

        for (uint8_t it = queue.begin(); it != queue.end();) {

            if (queue[it]->dst == nwk.src) {
                DataRequest *req = queue[it];
                it = queue.erase(it);
                cancel(req);
                forwardRequest(req);
            } else {
                it = queue.next(it);
            }
        }
    } else {
        node_t nextHop = getNextHop(nwk.dst);
        if (nextHop != BROADCAST ) {
            LOG_INFO("forward rrep to "<<nwk.dst<<" via "<<nextHop<<" from "<<nwk.src);
            // forward route reply
            DataRequest *req = new DataRequest(nextHop,
                    msg->decapsulateAirframe());
            req->getAirframe() << nwk;
#ifdef ROUTING_ENABLE_STATS
            control++;
#endif
            rrepReqOut.send(req);
        }
    }

    delete msg;
}

void AODV::handleRerrIndication(DataIndication* msg) {
    LOG_INFO("receive route error");

    delete msg;
}

void AODV::updateRoutingTable(node_t nextHop, node_t dst, uint8_t hops) {
    uint8_t it = table.find(dst);
    if (it != table.end()) {
        if (table[it].third >= hops) {
            table.erase(it);
            table.update(tableEntry_t(dst, nextHop, hops));
        }
    } else {
        table.update(tableEntry_t(dst, nextHop, hops));
    }
}

void AODV::handleIndication(DataIndication* msg) {
    NwkHeader nwk;
    msg->getAirframe() >> nwk;

    if (!checkHeader(nwk)) {
        delete msg;
        return;
    }

    updateRoutingTable(msg->src, nwk.src, nwk.hops);

    if (nwk.dst == palId_id()) {
        LOG_INFO("receive from "<<nwk.src);
        msg->src = nwk.src;
        msg->dst = nwk.dst;
        msg->set(new RoutingInfo(nwk.hops));
        sendIndication(msg);
        return;
    }

    // forward request
    DataRequest *req = new DataRequest(nwk.dst, msg->getAirframe().getCopy());
    req->getAirframe() << nwk;
    forwardRequest(req);

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif

    if (nwk.dst == BROADCAST ) {
        msg->src = nwk.src;
        msg->dst = nwk.dst;
        msg->set(new RoutingInfo(nwk.hops & 0x7f));
        sendIndication(msg);
    } else {
        delete msg;
    }

}

void AODV::handleResponse(DataResponse* resp) {
    DataRequest *req = ((RoutingRequestId*) resp->getRequestId())->req;

    // remove packet from queue not needed
    // queue.erase(queue.find(req));

    if (resp->isSuccess() == true) {
        req->response(new DataResponse(DataResponseStatus::SUCCESS));
    } else {
        NwkHeader nwk;
        req->getAirframe() >> nwk;
        table.erase(table.find(nwk.dst));
        std::cout << getId() << " DELETE ENTRY " << nwk.dst << " "
                << getNextHop(nwk.dst) << endl;
        req->response(new DataResponse(DataResponseStatus::FAIL_UNKNOWN));
    }

    delete req;
    delete resp;
}

void AODV::historyUpdate(Message* timer) {
    history.refresh();
    schedule(timer, &AODV::historyUpdate, ROUTING_HISTORY_REFRESH);
}

void AODV::tableUpdate(Message* timer) {
    table.refresh();
    schedule(timer, &AODV::tableUpdate, ROUTING_TABLE_REFRESH);
}

void AODV::finish() {
    for (uint8_t it = queue.begin(); it != queue.end(); it = queue.next(it)) {
        cancel(queue[it]);
        delete queue[it];
    }
    queue.clear();

#ifdef ROUTING_ENABLE_STATS
    recordScalar("forwarded", forwarded);
    recordScalar("control",control);
    recordScalar("queueOverflow",queueOverflow);
#endif
}

} /* namespace cometos */

