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

#include "TZTCATreeRouting.h"
#include "MacAbstractionBase.h"
#include "RoutingInfo.h"
#include "logging.h"
#include "palId.h"

static const uint16_t QUALITY_HYSTERESIS = 1000;

namespace cometos {

Define_Module(TZTCATreeRouting);

TZTCATreeRouting::TZTCATreeRouting() :
        isSink(false), parent(BROADCAST) {

}

void TZTCATreeRouting::handleRequest(DataRequest* msg) {
    if (isSink) {
        msg->response(new DataResponse(DataResponseStatus::SUCCESS));
        sendIndication(new DataIndication(msg->decapsulateAirframe(), palId_id(), palId_id()));
        delete msg;
    } else {
        Routing::handleRequest(msg);
    }
}

void TZTCATreeRouting::initialize() {
    Routing::initialize();

#ifdef OMNETPP
    CONFIG_NED(isSink);
#endif

    if (isSink) {
        parent = 0;
    }
}

void TZTCATreeRouting::handleIndication(DataIndication* pkt, NwkHeader& nwk) {
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

void TZTCATreeRouting::setNeighborhood(TCPWY* neighborhood) {
    this->neighborhood = neighborhood;
}

void TZTCATreeRouting::updateParent() {
    ASSERT(neighborhood != nullptr);

    uint16_t parentQuality = 0;
    uint16_t parentHops = 0xFFFF;

    node_t bestNewParent = BROADCAST;
    uint16_t bestQuality = 0;
    uint16_t bestHops = 0xFFFF;

    for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
        TZTCAElement& neighbor = neighborhood->tca.neighborView[i];
        if(neighbor.hasBidirectionalLink()) {
            if(neighbor.ccID != 0) {
                // Not in a connected component with the sink!
                continue;
            }

            if(neighbor.id == parent) {
                parentQuality = neighbor.qualityLT;
                parentHops = neighbor.ccDist;
            }

            if(neighbor.ccDist < bestHops || (neighbor.ccDist == bestHops && neighbor.qualityLT > bestQuality)) {
                bestNewParent = neighbor.id;
                bestQuality = neighbor.qualityLT;
                bestHops = neighbor.ccDist;
            }
        }
    }

    if((parentQuality == 0) ||    // Parent no longer has a bidirectional link 
       (bestHops < parentHops) || // Routing via parent is a longer way
       (bestHops == parentHops && bestQuality > parentQuality+QUALITY_HYSTERESIS)) { // Parent has a much lower quality
        LOG_INFO("New parent 0x" << cometos::hex << bestNewParent << " " << cometos::dec << bestQuality << " " << bestHops+1 << " old: 0x" << cometos::hex << parent << cometos::dec << " " << parentHops+1 << " " << parentQuality);
        parent = bestNewParent;
    }
}

void TZTCATreeRouting::handleRequest(DataRequest* msg, NwkHeader& nwk) {
    msg->getAirframe() << nwk; // add network header to packet
    if (nwk.dst == BROADCAST) {
        ASSERT(false); // TODO remove - only for current debugging
        msg->dst = BROADCAST;
    } else {
        updateParent();

        if (parent==BROADCAST) {
            LOG_ERROR("DROP ROUTING");
            msg->response(new DataResponse(DataResponseStatus::NO_ROUTE));
            delete msg;
            return;
        }
        msg->dst = parent;
    }

    timeOffset_t offset = 0;
    if (palId_id() != nwk.src && sendingOffset > 0 && msg->dst == BROADCAST) {
        offset = sendingOffset / 2 + intrand(sendingOffset);
    }

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif
    sendRequest(msg, offset);
}

} /* namespace cometos */
