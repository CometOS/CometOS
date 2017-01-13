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

#include "ShortCuttingTreeRouting.h"
#include "MacAbstractionBase.h"
#include "RoutingInfo.h"
#include "MacControl.h"
#include "logging.h"

/**duration of one slot, after each slot new parent and hop count is
 * used.
 */
#define DEFAULT_SLOT_DURATION	5000

#define BACKOFF_FOR_CONSECUTIVE_TRANSMISSION 20
#define QUEUE_SIZE 5

/**Number of beacons a node sends per slot. A value of 2-4 is
 * recommend to cope with packet loss.
 */
#define BEACONS_PER_SLOT 3

using namespace std;
using namespace cometos;
Define_Module(ShortCuttingTreeRouting);

ShortCuttingTreeRouting::ShortCuttingTreeRouting() :
        beaconIn(this, &ShortCuttingTreeRouting::handleBeacon, "beaconIn"), beaconOut(
                this, "beaconOut"), hops(0xff), nextHops(0xff), isSink(false), slotsToRun(
                0xffff) {
    parents.clear();
    nextParents.clear();

}
void ShortCuttingTreeRouting::finish() {
    Routing::finish();

#ifdef FIXED_BEACONING
    cancel(ttlTimer);
    delete ttlTimer;
#endif

    while (!queue.empty()) {
        delete *queue.begin();
        queue.pop_front();
    }

    //recordScalar("parent", parent);
    //recordScalar("hopsToSink", hops);

#ifdef ROUTING_ENABLE_STATS
    recordScalar("forwarded", forwarded);
    recordScalar("control",control);
    recordScalar("parentChanged",parentChanged);
    recordScalar("numControlRecv",numControlRecv);

    recordScalar("lossQueue",lossQueue);
    recordScalar("lossTransmission",lossTransmission);
    recordScalar("noRoute", noRoute);

#endif

}

void ShortCuttingTreeRouting::handleRequest(DataRequest* msg) {
    if (isSink) {
        msg->response(new DataResponse(DataResponseStatus::SUCCESS));
        sendIndication(new DataIndication(msg->decapsulateAirframe(), getId(), getId()));
        delete msg;
    } else {
        Routing::handleRequest(msg);
    }

}

void ShortCuttingTreeRouting::initialize() {
    Routing::initialize();

    slotDuration = DEFAULT_SLOT_DURATION;
    CONFIG_NED(slotDuration);
    CONFIG_NED(isSink);
    CONFIG_NED(slotsToRun);
    CONFIG_NED(broadcastTxPower);

#ifdef FIXED_BEACONING
    ttlTimer = new Message;
    ttlCounter=0;
 #endif

    // timer are set in such a way that in each slot at least one
    // beacon of each node can be received
    schedule(new Message, &ShortCuttingTreeRouting::sendBeacon,
            slotDuration / 2 + intrand(slotDuration / 2));
    schedule(new Message, &ShortCuttingTreeRouting::slotTimeout, slotDuration);

    isSending = false;
    sendingBackoff = 0;

    if (isSink) {
        parents.clear();
        hops = 0;
    } else {
        currentParent = BROADCAST;
        currentShortCutIndex = 0xFF;
    }

#ifdef ROUTING_ENABLE_STATS
    parentChanged=0;
    forwarded=0;
    control=0;
    numControlRecv=0;

    lossTransmission=0;
    lossQueue=0;
    noRoute=0;
#endif

}

void ShortCuttingTreeRouting::sendBeacon(Message *timer) {
    schedule(timer, &ShortCuttingTreeRouting::sendBeacon,
            slotDuration / (BEACONS_PER_SLOT * 2)
                    + intrand(slotDuration / BEACONS_PER_SLOT));

    if (slotsToRun == 0) {
        return;
    }

#ifdef ROUTING_ENABLE_STATS
    control++;
#endif

    LOG_INFO("send beacon");
    AirframePtr air = make_checked<Airframe>();
    (*air) << hops << parents;
    DataRequest *req = new DataRequest(BROADCAST, air);
    req->set(new MacControl(broadcastTxPower));
    beaconOut.send(req);

}

#ifdef FIXED_BEACONING
void ShortCuttingTreeRouting::ttlTimeout(Message *timer) {
    ttlCounter++;
    if (ttlCounter==3) {
        nextParents.clear();
        nextHops = 0xff;
        ttlCounter=0;
    }

}

#endif

void ShortCuttingTreeRouting::slotTimeout(Message *timer) {
    schedule(timer, &ShortCuttingTreeRouting::slotTimeout, slotDuration);

    if (slotsToRun == 0) {
        return;
    }
    if (slotsToRun != 0xFFFF) {
        slotsToRun -= 1;
    }

    parents = nextParents;
    hops = nextHops;

#ifndef FIXED_BEACONING
    nextParents.clear();
    nextHops = 0xff;
#endif

    if (isSink) {
        parents.clear();
        hops = 0;
    }

    if (parents.getSize() > 0) {
        // delete shortcut parent, if parent changes
        if (currentParent != parents[0]) {

            // the index is not changed, the value is decreased and randomly increased during runtime
            //currentShortCutIndex = 0xff;

#ifdef ROUTING_ENABLE_STATS
            parentChanged++;
#endif
        }
        currentParent = parents[0];
    } else {
        currentParent = BROADCAST;
    }

    /*   cout << getId() << " ->" << endl;
     if (parents.getSize() > 0) {
     for (uint8_t i = 0; i < parents.getSize(); i++) {
     cout << "    parent " << parents[i] << " hops " << (int) (i + 1)
     << endl;
     }
     }
     */
    sendNext();
}

void ShortCuttingTreeRouting::handleBeacon(DataIndication* pkt) {

    uint8_t tempHops;

#ifdef ROUTING_ENABLE_STATS
    numControlRecv++;
#endif

    parentList_t list;

    pkt->getAirframe() >> list >> tempHops;

    LOG_INFO("process beacon from "<<cometos::hex<<pkt->src<<cometos::dec);

#ifdef FIXED_BEACONING

    // if waiting for a long time for an update of a node (but non is receive), make a soft transition to another node
    if ((tempHops + 1) < (nextHops)
            || (nextParents.getSize() > 0 && nextParents[0] == pkt->src) || ttlCounter==2) {

        cancel(ttlTimer);
        schedule(ttlTimer, &ShortCuttingTreeRouting::ttlTimeout,slotDuration);
        ttlCounter=0;

        nextHops = tempHops + 1;
        nextParents.clear();
        nextParents.pushBack(pkt->src);
        ASSERT((nextParents.getSize() > 0 && nextParents[0] == pkt->src));
        for (uint8_t i = 0;
                i < (nextParents.getMaxSize() - 1) && i < list.getSize(); i++) {
            nextParents.pushBack(list[i]);

            LOG_DEBUG("   reg: "<<(int)i<<cometos::hex<<" "<<list[i]<<cometos::dec);
        }
    }
#else

    if ((tempHops + 1) < (nextHops)
            || ((tempHops + 1) <= (nextHops) && currentParent == pkt->src)) {

        nextHops = tempHops + 1;
        nextParents.clear();
        nextParents.pushBack(pkt->src);
        for (uint8_t i = 0;
                i < (nextParents.getMaxSize() - 1) && i < list.getSize(); i++) {
            nextParents.pushBack(list[i]);

            LOG_DEBUG("   reg: "<<(int)i<<hex<<" "<<list[i]<<dec);
        }
    }
#endif

    if ((tempHops + 1) < (nextHops)
            || ((tempHops + 1) <= (nextHops) && currentParent == pkt->src)) {

        nextHops = tempHops + 1;
        nextParents.clear();
        nextParents.pushBack(pkt->src);
        for (uint8_t i = 0;
                i < (nextParents.getMaxSize() - 1) && i < list.getSize(); i++) {
            nextParents.pushBack(list[i]);

            LOG_DEBUG("   reg: "<<(int)i<<cometos::hex<<" "<<list[i]<<cometos::dec);
        }
    }

    delete pkt;

}

void ShortCuttingTreeRouting::handleIndication(DataIndication* pkt,
        NwkHeader& nwk) {

    LOG_INFO("receive data");
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

void ShortCuttingTreeRouting::handleRequest(DataRequest* msg, NwkHeader& nwk) {
    msg->getAirframe() << nwk; // add network header to packet

    // BROADCAST NOT SUPPORTED
    ASSERT(nwk.dst != BROADCAST);

    if (queue.size() >= QUEUE_SIZE) {
        msg->response(new DataResponse(DataResponseStatus::QUEUE_FULL));
#ifdef ROUTING_ENABLE_STATS
        lossQueue++;
#endif
        delete msg;
        return;
    }

    queue.push_back(msg);

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif

    sendNext();

}

void ShortCuttingTreeRouting::sendNext() {

    if (!isSending && queue.size() > 0 && currentParent == BROADCAST ) {
        LOG_WARN("no route to sink, sending failed");

#ifdef ROUTING_ENABLE_STATS
        noRoute++;
#endif
        return;
    }

    if (isSending || queue.size() == 0 || currentParent == BROADCAST ) {
        return;
    }

    isSending = true;

    ASSERT(parents.getSize()>0);

    /*optional improvement*/
    if (intrand(100) <= INCREASE_PARENT_PROB) {
        if (currentShortCutIndex < parents.getSize()) {
            LOG_WARN("increase parent again");
            currentShortCutIndex++;
        }
    }

    if (currentShortCutIndex >= parents.getSize()) {
        currentShortCutIndex = parents.getSize() - 1;
    }

    LOG_WARN("forward data via "<<cometos::hex<<parents[currentShortCutIndex]<<cometos::dec<<" index "<<(int)currentShortCutIndex);
    ASSERT(queue.size()>0);
    ASSERT(*queue.begin()!=NULL);
    ASSERT(currentShortCutIndex<parents.getSize());

    DataRequest *req = new DataRequest(parents[currentShortCutIndex],
            AirframePtr((*queue.begin())->getAirframe().getDeepCopy()),
            createCallback(&ShortCuttingTreeRouting::handleResponse));
    sendRequest(req, sendingBackoff);
    sendingBackoff = 0;
}

void ShortCuttingTreeRouting::handleResponse(DataResponse* response) {
    //std::cout<<"SEND "<<simTime().dbl()<< " with INDEX " <<(int)currentShortCutIndex <<std::endl;
    isSending = false;

    if (!response->isSuccess()) {
        if (currentShortCutIndex == 0) {
            //currentShortCutIndex = 0xff;
            //currentParent = BROADCAST;

            // delete packet to avoid accumulation of packets
            (*queue.begin())->response(new DataResponse(response->status));
            delete *queue.begin();
            queue.pop_front();

        } else {
            currentShortCutIndex--;
        }LOG_WARN("no response reduce shortcut index to "<< (int)currentShortCutIndex);
#ifdef ROUTING_ENABLE_STATS
        lossTransmission++;
#endif

    } else {
        LOG_WARN("sending succeed");
        ASSERT(queue.size()>0);
        (*queue.begin())->response(new DataResponse(DataResponseStatus::SUCCESS));
        delete *queue.begin();
        queue.pop_front();

    }

    delete response;

    // TO TEST
    if (sendingBackoff == 0) {
        sendingBackoff = intrand(BACKOFF_FOR_CONSECUTIVE_TRANSMISSION);
    }
    sendNext();

}

