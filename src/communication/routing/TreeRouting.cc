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

#include "TreeRouting.h"
#include "MacAbstractionBase.h"
#include "RoutingInfo.h"
#include "logging.h"

/**duration of one slot, after each slot new parent and hop count is
 * used.
 */
#define DEFAULT_SLOT_DURATION	5000

/**Number of beacons a node sends per slot. A value of 2-4 is
 * recommend to cope with packet loss.
 */
#define BEACONS_PER_SLOT 3

/**Minimum lqi value of beacons. Beacons with lower value are filtered out.
 */
#define LQI_FILTER 210

namespace cometos {

Define_Module(TreeRouting);

TreeRouting::TreeRouting() :
        beaconIn(this, &TreeRouting::handleBeacon, "beaconIn"), beaconOut(this,
                "beaconOut"), parent(BROADCAST ), hops(0xff), nextParent(
                BROADCAST ), nextHops(0xff), isSink(false), slotsToRun(0xffff) {

}
void TreeRouting::finish() {
    Routing::finish();
    recordScalar("parent", parent);recordScalar("hopsToSink", hops);

#ifdef ROUTING_ENABLE_STATS
    recordScalar("forwarded", forwarded);
    recordScalar("control",control);
    recordScalar("numControlRecv",numControlRecv);
#endif

}

void TreeRouting::handleRequest(DataRequest* msg) {
    if (isSink) {
        msg->response(new DataResponse(DataResponseStatus::SUCCESS));
        sendIndication(new DataIndication(msg->decapsulateAirframe(), getId(), getId()));
        delete msg;
    } else {
        Routing::handleRequest(msg);
    }

}

void TreeRouting::initialize() {
    Routing::initialize();

    slotDuration = DEFAULT_SLOT_DURATION;
    CONFIG_NED(slotDuration);CONFIG_NED(isSink);CONFIG_NED(slotsToRun);

    // timer are set in such a way that in each slot at least one
    // beacon of each node can be received
    schedule(new Message, &TreeRouting::sendBeacon,
            slotDuration / 2 + intrand(slotDuration / 2));
    schedule(new Message, &TreeRouting::slotTimeout, slotDuration);

    if (isSink) {
        parent = 0;
        hops = 0;
    }

#ifdef ROUTING_ENABLE_STATS
    forwarded=0;
    control=0;
    numControlRecv=0;
#endif

}

void TreeRouting::sendBeacon(Message *timer) {
    schedule(timer, &TreeRouting::sendBeacon,
            slotDuration / (BEACONS_PER_SLOT * 2)
                    + intrand(slotDuration / BEACONS_PER_SLOT));

    if (slotsToRun == 0) {
        return;
    }LOG_INFO("send beacon");

    AirframePtr air = make_checked<Airframe>();
    (*air) << hops;

#ifdef ROUTING_ENABLE_STATS
    control++;
#endif

    beaconOut.send(new DataRequest(BROADCAST, air));

}

void TreeRouting::slotTimeout(Message *timer) {
    schedule(timer, &TreeRouting::slotTimeout, slotDuration);

    if (slotsToRun == 0) {
        return;
    }
    if (slotsToRun != 0xFFFF) {
        slotsToRun -= 1;
    }

    parent = nextParent;
    hops = nextHops;
    nextParent = BROADCAST;
    nextHops = 0xff;
    if (isSink) {
        parent = 0;
        hops = 0;
    }

    LOG_INFO("parent="<<parent<<" hops="<<(int)hops);
}

void TreeRouting::handleBeacon(DataIndication* pkt) {

#ifdef ROUTING_ENABLE_STATS
    numControlRecv++;
#endif

    // only accept packets of high quality
    MacRxInfo* phy = pkt->get<MacRxInfo>();
    if (phy->lqi < LQI_FILTER) {
        delete pkt;
        return;
    }

    uint8_t tempHops;

    pkt->getAirframe() >> tempHops;

    LOG_INFO("process beacon from "<<pkt->src);

    if ((tempHops + 1) < (nextHops)) {
        nextHops = tempHops + 1;
        nextParent = pkt->src;
    }
    delete pkt;
}

void TreeRouting::handleIndication(DataIndication* pkt, NwkHeader& nwk) {

    nwk.hops++;
    if (isSink) {
        pkt->dst = nwk.dst;
        pkt->src = nwk.src;
        pkt->set(new RoutingInfo(nwk.hops));
        sendIndication(pkt);
    } else {
        handleRequest(new DataRequest(nwk.dst, pkt->decapsulateAirframe()),
                nwk); // create request for sending
        delete pkt;
    }
}

void TreeRouting::handleRequest(DataRequest* msg, NwkHeader& nwk) {
    msg->getAirframe() << nwk; // add network header to packet
    if (nwk.dst == BROADCAST ) {
        msg->dst = BROADCAST;
    } else {
        if (parent==BROADCAST) {
            msg->response(new DataResponse(DataResponseStatus::NO_ROUTE));
            delete msg;
            return;
        }
        msg->dst = parent;
    }

    timeOffset_t offset = 0;
    if (getId() != nwk.src && sendingOffset > 0 && msg->dst == BROADCAST ) {
        offset = sendingOffset / 2 + intrand(sendingOffset);
    }

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif
    sendRequest(msg, offset);
}

} /* namespace cometos */
