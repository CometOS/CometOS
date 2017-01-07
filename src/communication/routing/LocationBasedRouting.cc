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
#include "palId.h"

namespace cometos {

Define_Module(LocationBasedRouting);

Airframe& operator<<(Airframe& frame, LocationBasedRoutingHeader& value) {
    return frame << value.nlSrc << value.nlDst;
}

Airframe& operator>>(Airframe& frame, LocationBasedRoutingHeader& value) {
    return frame >> value.nlDst >> value.nlSrc;
}

LocationBasedRouting::LocationBasedRouting()
{
}

void LocationBasedRouting::handleRequest(DataRequest* msg) {
    // from upper layer
    ASSERT(msg->dst != MAC_BROADCAST); // do not allow network broadcasts
    LocationBasedRoutingHeader hdr;
    hdr.nlDst = msg->dst;
    hdr.nlSrc = palId_id();
    forwardRequest(msg, hdr);
}

void LocationBasedRouting::forwardRequest(DataRequest* msg, LocationBasedRoutingHeader& hdr) {
    getCout() << "handleRequest at " << palId_id() << " to " << hdr.nlDst << " --------------- \t ";

    Coordinates destinationCoordinates = PalLocation::getInstance()->getCoordinatesForNode(hdr.nlDst);
    if(destinationCoordinates == Coordinates::INVALID_COORDINATES) {
        getCout() << " throw away!" << endl;
        msg->response(new DataResponse(DataResponseStatus::INVALID_ADDRESS));
        delete msg;
        return;
    }

    node_t nextHop = MAC_BROADCAST;
    CoordinateType minDistance = destinationCoordinates.getSquaredDistance(PalLocation::getInstance()->getOwnCoordinates());

    ASSERT(neighborhood != nullptr);
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
        if(neighborhood->tca.neighborView[i].hasBidirectionalLink()) {
            node_t id = neighborhood->tca.neighborView[i].id;
            //getCout() << id << "/" << coordinates.x << "/" << coordinates.y << "-";
            if(id == hdr.nlDst) {
                // direct neighbor, just send
                nextHop = id;
                break;
            }
            else {
                Coordinates coordinates = PalLocation::getInstance()->getCoordinatesForNode(id);
                if(coordinates != Coordinates::INVALID_COORDINATES) {
                    CoordinateType distance = destinationCoordinates.getSquaredDistance(coordinates);
                    if(distance < minDistance) {
                        nextHop = id;
                        minDistance = distance;
                    }
                }
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
        msg->getAirframe() << hdr;
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
    LocationBasedRoutingHeader hdr;
    pkt->getAirframe() >> hdr;
    getCout() << "handleIndication from " << pkt->src << " to " << pkt->dst << " at " << palId_id() << endl;

    if (palId_id() == hdr.nlDst) {
        pkt->src = hdr.nlSrc;
        pkt->dst = hdr.nlDst;
        sendIndication(pkt);
    } else {
        forwardRequest(new DataRequest(0 /* is ignored */, pkt->decapsulateAirframe()), hdr);
        delete pkt;
    }
}

} /* namespace cometos */

