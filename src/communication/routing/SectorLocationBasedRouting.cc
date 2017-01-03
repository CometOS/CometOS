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

#ifndef NO_SERIALIZABLE_WRAPPER

#include "SectorLocationBasedRouting.h"
#include "MacAbstractionBase.h"
#include "palLocation.h"
#include "palId.h"
#include "math.h"
#include "RoutingInfo.h"
#include "logging.h"

/**maximum number of sectors, this number should be equal*/
#define MAX_SECTORS 16

/**maximum and required number candidates*/
#define REQUIRED_CANDIDATES 5

/**maximum queue size*/
#define QUEUE_SIZE 20

// in mm
#define DIRECT_TRANSMISSION_RANGE 40000

#define LQI_FILTER 210

#define REPLY_OFFSET    40

using namespace std;

namespace cometos {

std::map<int, position_t> SectorLocationBasedRouting::locationTable;

Define_Module(SectorLocationBasedRouting);

SectorLocationBasedRouting::SectorLocationBasedRouting() :
        nextHops(MAX_SECTORS, BROADCAST ), currentRequest(NULL), sectorRequestIn(
                this, &SectorLocationBasedRouting::handleSectorRequest,
                "sectorRequestIn"), sectorRequestOut(this, "sectorRequestOut"), sectorReplyIn(
                this, &SectorLocationBasedRouting::handleSectorReply,
                "sectorReplyIn"), sectorReplyOut(this, "sectorReplyOut")

{
    sequence = 0;
}

uint8_t SectorLocationBasedRouting::getSector(node_t dst, bool inverse) {

    ASSERT(locationTable.count(dst));

    position_t dstPosition = locationTable[dst];

    double dx = dstPosition.first - ownPosition.first;
    double dy = dstPosition.second - ownPosition.second;
    if (inverse) {
        dx = -dx;
        dy = -dy;

    }

    double angle;

    if (dx == 0) {
        if (dy > 0) {
            angle = M_PI / 2;
        } else {
            angle = -M_PI / 2;
        }
    } else if (dx > 0) {
        angle = atan(dy / dx);
    } else {
        angle = M_PI + atan(dy / dx);
    }

    // LOG_INFO(" angle "<<dst << " " <<angle);
    angle += M_PI / MAX_SECTORS; // centering node in the middle of the sector search

    if (angle < 0) {
        angle += 2 * M_PI;
    }

    ASSERT(angle<(2 * M_PI));
    uint8_t res = (angle / (2 * M_PI)) * MAX_SECTORS;
    ASSERT(res< MAX_SECTORS);
    return res;
}

double SectorLocationBasedRouting::getDistance(node_t dst) {

    ASSERT(locationTable.count(dst));

    position_t dstPosition = locationTable[dst];

    double dx = dstPosition.first - ownPosition.first;
    double dy = dstPosition.second - ownPosition.second;

    return sqrt(dx * dx + dy * dy);

}

void SectorLocationBasedRouting::startProcessing() {

    if (NULL != currentRequest || queue.size() == 0) {
        return;
    }

    currentRequest = queue.front();
    queue.pop_front();
    candidates.clear();

    currentSectors.first = getSector(currentRequest->dst);
    ASSERT( currentSectors.first <MAX_SECTORS);
    currentSectors.second = (currentSectors.first + 1) % MAX_SECTORS;

    LOG_WARN("forward to target "<<currentRequest->dst<<" in sector "<<(int)currentSectors.first);

    forward();

}

void SectorLocationBasedRouting::forward() {
    forceTransmission = false;
    if (getDistance(currentRequest->dst) <= DIRECT_TRANSMISSION_RANGE) {
        LOG_INFO("use direct transmission");

        LOG_INFO("send direct");

#ifdef ROUTING_ENABLE_STATS
        forwarded++;
#endif
        sendRequest(
                new DataRequest(currentRequest->dst,
                        currentRequest->getAirframe().getDeepCopy(),
                        createCallback(
                                &SectorLocationBasedRouting::handleResponseDirect)));

        return;
    }

    // search for destination to send packet to
    ASSERT(currentSectors.first<MAX_SECTORS);
    for (uint8_t i = currentSectors.first; i != currentSectors.second;
            i = (i + 1) % MAX_SECTORS) {
        if (nextHops[i] != BROADCAST ) {
            currentCandidate = nextHops[i];

            LOG_INFO("send to forwarder "<< currentCandidate);
            double backoff = 0;
            if (!establishedMap[nextHops[i]]) {
                // high values can significantly improve performance
                backoff = REPLY_OFFSET * 5;
                establishedMap[nextHops[i]] = true;
            }

#ifdef ROUTING_ENABLE_STATS
            forwarded++;
#endif

            //cout << " " << backoff << endl;
            sendRequest(
                    new DataRequest(nextHops[i],
                            currentRequest->getAirframe().getDeepCopy(),
                            createCallback(
                                    &SectorLocationBasedRouting::handleResponse)),
                    backoff);

            return;
        }
    }

    Airframe *frame = new Airframe();
    (*frame) << ownPosition << currentSectors << currentRequest->dst;

#ifdef ROUTING_ENABLE_STATS
    control++;
    discoveries++;
#endif

    LOG_INFO("send request");
    sectorRequestOut.send(
            new DataRequest(BROADCAST, frame,
                    createCallback(
                            &SectorLocationBasedRouting::handleResponseRequest)));

    waitingForReply = true;

}

void SectorLocationBasedRouting::handleDiscoveryTimeout(Message* msg) {

    delete msg;
    waitingForReply = false;

    // not sufficient forwarders received
    if (candidates.size() < REQUIRED_CANDIDATES) {

        // node was not able to send packet
        if (forceTransmission) {
            forceTransmission = false;
            finishProcessing(false);
            LOG_ERROR("failed to forward");
            return;
        }

        LOG_INFO("insufficient repliers "<<candidates.size());
        currentSectors.first = (currentSectors.first - 1 + MAX_SECTORS)
                % MAX_SECTORS;
        currentSectors.second = (currentSectors.second + 1) % MAX_SECTORS;

        // old candidates stay valid
        //candidates.clear();

        if ((((MAX_SECTORS + currentSectors.second - currentSectors.first)
                % MAX_SECTORS) >= (MAX_SECTORS / 2))) {
            if (candidates.size() > 0) {
                LOG_WARN("found not enough forwarders, force transmission");
                forceTransmission = true;
                sendToCandidate();
            } else {
                finishProcessing(false);
                LOG_ERROR("failed to forward");
            }
            // try at least sending to one candidate
            return;
        }
        forward();

        return;
    }
    //LOG_ERROR("Send to candidate");
    sendToCandidate();

}

void SectorLocationBasedRouting::sendToCandidate() {
    double dist = 0;
    node_t dst = BROADCAST;

    // if candidate list becomes too short, search for new candidate
    if (candidates.size() < REQUIRED_CANDIDATES && !forceTransmission) {
        handleDiscoveryTimeout(new Message);
        return;
    }

    for (map<node_t, double>::iterator it = candidates.begin();
            it != candidates.end(); it++) {
        if (it->second > dist) {
            dist = it->second;
            dst = it->first;
        }
    }

    if (dst == BROADCAST ) {
        finishProcessing(false);
        return;
    }
    currentCandidate = dst;

#ifdef ROUTING_ENABLE_STATS
    forwarded++;
#endif
    LOG_INFO("send reply");

    sendRequest(
            new DataRequest(dst, currentRequest->getAirframe().getDeepCopy(),
                    createCallback(
                            &SectorLocationBasedRouting::handleResponseCandidate)));

}

void SectorLocationBasedRouting::finishProcessing(bool success) {
    ASSERT(currentRequest!=NULL);
    currentRequest->response(new DataResponse(success ? DataResponseStatus::SUCCESS : DataResponseStatus::FAIL_UNKNOWN));
    delete currentRequest;
    currentRequest = NULL;
    startProcessing();
}

void SectorLocationBasedRouting::handleRequest(DataRequest* msg,
        NwkHeader& nwk) {

    nwk.hops++;
    msg->getAirframe() << nwk;
    msg->dst = nwk.dst;

    if (queue.size() >= QUEUE_SIZE || msg->dst == BROADCAST ) {
        msg->response(new DataResponse(DataResponseStatus::QUEUE_FULL));
        LOG_WARN("queue overflow, discard packet");
        delete msg;
        return;
    }
    queue.push_back(msg);

    startProcessing();

}

void SectorLocationBasedRouting::finish() {
    Layer::finish();

    memCheck1.check();
    memCheck2.check();

#ifdef ROUTING_ENABLE_STATS
    recordScalar("control",control);
    recordScalar("discoveries",discoveries);
    recordScalar("forwarded",forwarded);
#endif

    if (currentRequest) {
        delete currentRequest;
    }

    while (!queue.empty()) {
        delete *queue.begin();
        queue.pop_front();
    }
}

void SectorLocationBasedRouting::initialize() {
    Layer::initialize();

    schedule(new Message, &SectorLocationBasedRouting::historyUpdate);

    forceTransmission = false;

#ifdef ROUTING_ENABLE_STATS
    control=0;
    forwarded=0;
    discoveries=0;
#endif

    ownPosition.first = PalLocation::getInstance()->getOwnCoordinates().x;
    ownPosition.second = PalLocation::getInstance()->getOwnCoordinates().y;

    waitingForReply = false;

    alreadySendingReply = 0;

    locationTable[palId_id() ] = ownPosition;

    LOG_INFO("initialize "<<palId_id()<<" "<<ownPosition.first<<" "<<ownPosition.second);

}

void SectorLocationBasedRouting::handleSectorRequest(DataIndication* pkt) {

    MacRxInfo* phy = pkt->get<MacRxInfo>();
    if (phy->lqi < LQI_FILTER || alreadySendingReply >= MAX_REPLIES) {
        delete pkt;
        return;
    }

    // TODO this should be done with received location
    uint8_t requestedSector = getSector(pkt->src, true);

    sectorSpan_t senderSpan;
    position_t senderPos;
    node_t dst;
    pkt->getAirframe() >> dst >> senderSpan >> senderPos;

    // check whether route to destination is already established
    uint8_t targetSector = getSector(pkt->src);
    ASSERT(targetSector<MAX_SECTORS);
    bool established = nextHops[targetSector] != BROADCAST;

    LOG_INFO("process span "<< (int)senderSpan.first<<" to " << (int)senderSpan.second<<" own sector "<<(int)requestedSector);

    for (uint8_t i = senderSpan.first; i != senderSpan.second;
            i = (i + 1) % MAX_SECTORS) {
        if (i == requestedSector) {
            LOG_INFO("reply to discovery message");
            Airframe *frame = new Airframe;
            (*frame) << ownPosition << established;

#ifdef ROUTING_ENABLE_STATS
            control++;
#endif

            ASSERT(alreadySendingReply<MAX_REPLIES);
            alreadySendingReply++;
            LOG_INFO("send reply ");
            sectorReplyOut.send(
                    new DataRequest(pkt->src, frame,
                            createCallback(
                                    &SectorLocationBasedRouting::handleResponseReply)),
                    intrand(REPLY_OFFSET));
            break;
        }
    }

    delete pkt;
}

void SectorLocationBasedRouting::handleIndication(DataIndication* pkt,
        NwkHeader& nwk) {

    LOG_INFO("receive message for "<<nwk.dst);

    if (getId() == nwk.dst) {
        pkt->dst = nwk.dst;
        pkt->src = nwk.src;
        pkt->set(new RoutingInfo(nwk.hops));
        sendIndication(pkt);
    } else {
        handleRequest(new DataRequest(nwk.dst, pkt->decapsulateAirframe()),
                nwk);
        delete pkt;
    }
}

void SectorLocationBasedRouting::handleSectorReply(DataIndication* pkt) {
    LOG_INFO("receive reply "<<pkt->src << " (waiting "<<waitingForReply <<")");
    if (pkt->dst == getId() && waitingForReply) {
        position_t posDst;
        bool established;
        pkt->getAirframe() >> established >> posDst;
        addCandidate(pkt->src, posDst, established);
    }
    delete pkt;
}

void SectorLocationBasedRouting::addCandidate(node_t dst, position_t& posDst,
        bool established) {
    candidates[dst] = getDistance(dst);
    establishedMap[dst] = established;
}

void SectorLocationBasedRouting::handleResponseCandidate(DataResponse *resp) {
    if (!resp->isSuccess()) {
        candidates.erase(currentCandidate);
        sendToCandidate();
    } else {
        LOG_INFO("store candidate");

        ASSERT(currentSectors.first<MAX_SECTORS);
        for (uint8_t i = currentSectors.first; i != currentSectors.second;
                i = (i + 1) % MAX_SECTORS) {
            if (nextHops[i] == BROADCAST ) {
                nextHops[i] = currentCandidate;
            }
        }LOG_INFO("set forwarder to "<<currentCandidate);
        finishProcessing(true);
        currentCandidate = BROADCAST;
    }

    delete resp;
}

void SectorLocationBasedRouting::handleResponse(DataResponse *resp) {
    if (!resp->isSuccess()) {
        LOG_ERROR("Sending failed");

        ASSERT(currentSectors.first<MAX_SECTORS);

        // delete all occurrences of forwarding node
        for (uint8_t i = 0; i < MAX_SECTORS; i++) {
            if (nextHops[i] == currentCandidate) {
                nextHops[i] = BROADCAST;
            }
        }

        currentCandidate = BROADCAST;
        forward();
    } else {
        LOG_INFO("send succeed ");
        /*   ASSERT(currentSectors.first<MAX_SECTORS);
         for (uint8_t i = currentSectors.first; i != currentSectors.second;
         i = (i + 1) % MAX_SECTORS) {
         if (nextHops[i] == BROADCAST ) {
         nextHops[i] = currentCandidate;
         }
         }*/

        finishProcessing(true);
    }

    delete resp;
}

void SectorLocationBasedRouting::handleResponseDirect(DataResponse *resp) {
    if (!resp->isSuccess()) {
        LOG_ERROR("Sending direct failed");
    }

    finishProcessing(resp->isSuccess());
    delete resp;
}

void SectorLocationBasedRouting::handleRequest(DataRequest* msg) {
    NwkHeader nwk(msg->dst, palId_id(), ++sequence); // generate network header
    history.update(historyEntry_t(nwk.src, nwk.seq));
    handleRequest(msg, nwk); // forward to next processing unit
}

void SectorLocationBasedRouting::handleIndication(DataIndication* msg) {
    NwkHeader nwk;
    msg->getAirframe() >> nwk; // decapsulate header

    if (history.update(historyEntry_t(nwk.src, nwk.seq))) {
        delete msg;
        return;
    }

    handleIndication(msg, nwk);

}

void SectorLocationBasedRouting::handleResponseReply(DataResponse *resp) {
    ASSERT(alreadySendingReply>0);
    alreadySendingReply--;
    delete resp;
}

void SectorLocationBasedRouting::historyUpdate(Message* timer) {
    history.refresh();
    schedule(timer, &SectorLocationBasedRouting::historyUpdate,
            ROUTING_HISTORY_REFRESH);
}

void SectorLocationBasedRouting::handleResponseRequest(DataResponse *resp) {

    schedule(new Message, &SectorLocationBasedRouting::handleDiscoveryTimeout,
            REPLY_OFFSET);
    delete resp;
}

} /* namespace cometos */
#endif
