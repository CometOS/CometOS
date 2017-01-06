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

#include "LocationBasedRouting.h"
#include "palLocation.h"

namespace cometos {

Define_Module(LocationBasedRouting);

LocationBasedRouting::LocationBasedRouting()
{
}

void LocationBasedRouting::handleRequest(DataRequest* msg) {
    ASSERT(msg->dst != MAC_BROADCAST); // do not allow network broadcasts

    getCout() << "handleRequest at " << palId_id() << " to " << msg->dst << " --------------- \t ";

    node_t nextHop = MAC_BROADCAST;

    ASSERT(neighborhood != nullptr);
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
        if(neighborhood->tca.neighborView[i].hasBidirectionalLink()) {
            node_t id = neighborhood->tca.neighborView[i].id;
            getCout() << id << " ";
            if(id == msg->dst) {
                // direct neighbor, just send
                nextHop = id;
                break;
            }
        }
    }

    if(nextHop == MAC_BROADCAST) { // no next hop found
        getCout() << " throw away!" << endl;
        msg->response(new DataResponse(DataResponseStatus::NO_ROUTE));
        delete msg;
    }
    else {
        getCout() << " next hop " << nextHop;
        getCout() << endl;
        msg->getAirframe() << msg->dst;
        msg->dst = nextHop;
        sendRequest(msg);
    }

}

void LocationBasedRouting::finish() {
    Layer::finish();
}

void LocationBasedRouting::initialize() {
    Layer::initialize();

#ifdef OMNETPP
    omnetpp::cModule* module = (omnetpp::cSimulation::getActiveSimulation())->getContextModule();
    omnetpp::cModule* neighborhoodMod = NULL;

    while(module) {
        neighborhoodMod = module->getSubmodule("neighborhood");
        if(neighborhoodMod) {
            break;
        }
        module = module->getParentModule();
    }

    neighborhood = dynamic_cast<TCPWY*>(neighborhoodMod);
#endif
}

void LocationBasedRouting::setNeighborhood(TCPWY* neighborhood) {
    this->neighborhood = neighborhood;
}

void LocationBasedRouting::handleIndication(DataIndication* pkt) {
    ASSERT(pkt->dst == palId_id() || pkt->dst == MAC_BROADCAST);
    pkt->getAirframe() >> pkt->dst;
    getCout() << "handleIndication from " << pkt->src << " to " << pkt->dst << endl;

    if (palId_id() == pkt->dst) {
        sendIndication(pkt);
    } else {
        ASSERT(pkt->dst != MAC_BROADCAST); // do not allow network broadcasts
        handleRequest(new DataRequest(pkt->dst, pkt->decapsulateAirframe()));
        delete pkt;
    }
}

} /* namespace cometos */

