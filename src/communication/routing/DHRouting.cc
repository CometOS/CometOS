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

/*INCLUDES-------------------------------------------------------------------*/
#include "DHRouting.h"
#include "MacAbstractionBase.h"
#include "RoutingInfo.h"
#include "OutputStream.h"

namespace cometos {
//#define LOG0(x) if (getId()==0 && 	 (cSimulation::getActiveSimulation()->getSimTime().dbl()>90)) {getCout()<<getId()<<":"<<x<<endl;}
//#define LOG5(x) if (getId()==3 && 	 (cSimulation::getActiveSimulation()->getSimTime().dbl()>90)) {getCout()<<getId()<<":"<<x<<endl;}
//#define LOG2(x) if (getId()==2 && 	 (cSimulation::getActiveSimulation()->getSimTime().dbl()>90)) {getCout()<<getId()<<":"<<x<<endl;}

#define GET_TIME() (cSimulation::getActiveSimulation()->getSimTime().dbl())

#define COUT_AFTER_WARMUP(x) if ( GET_TIME() > maxDiscoveryTime) getCout()<<getId()<<" "<<x<<endl

#define NB_RADIUS 3
#define MIS_RADIUS 1

// was 210
#define LQI_FILTER 210

/*METHOD DEFINITION----------------------------------------------------------*/

// this macro allows accessing this module in OMNETPP
Define_Module(DHRouting);

DHRoutingLayer::DHRoutingLayer(uint8_t nbRadius, uint8_t misRadius) :
        next(NULL), nbRadius(nbRadius), misRadius(misRadius), round(
                0) {

}

DHRoutingLayer::~DHRoutingLayer() {
    if (next != NULL) {
        delete next;
    }
}

node_t DHRoutingLayer::getLowestNbId() {
    node_t min = 0xFFFF;
    for (uint8_t it = nbs.begin(); it != nbs.end(); it = nbs.next(it)) {
        if (nbs[it].id < min && nbs[it].hops <= misRadius) {
            min = nbs[it].id;
        }
    }

    return min;
}

DataRequest* DHRoutingLayer::run(bool permitted, node_t ownId, uint8_t level) {

//	getCout() << ownId << " ExE " << (int) level << " permitted " << (int) permitte	<< endl;

    uint8_t index = round;

    round++;
    if (round >= DHROUING_MAX_NBS) {
        round = 0;
    }

    if (index == 0) {
        updateNbs();
    }

    node_t min = getLowestNbId();
    ASSERT(min!=ownId);

    if (next == NULL) {

        // Note that this also includes the spawn of a new layer if no neighbor is available
        if (permitted && (ownId < min)) {
            next = new DHRoutingLayer(nbRadius, misRadius);
            //getCout() << "Spawn new Cluster " << ownId << endl;
        } else {
            return NULL;
        }
    } else {
        if ((level > 0) && (ownId > min || !permitted)) {
            delete next;
            next = NULL;
            return NULL;
        }

    }

    DataRequest * ret = NULL;

    node_t node = getNeighbor(index);

    if (node != BROADCAST ) {
        AirframePtr air = make_checked<Airframe>();
        for (uint8_t it = next->nbs.begin(); it != next->nbs.end();
                it = next->nbs.next(it)) {
            (*air) << next->nbs[it].id << (uint8_t) next->nbs[it].hops;
        }

        (*air) << (uint8_t) next->nbs.size() << (uint8_t) (next->next != NULL);
        //getCout() << "node " << ownId << " sends " << (int) next->nbs.size()	<< " data" << endl;
        ret = new DataRequest(node, air);

    }

    return ret;

}

void DHRoutingLayer::deleteNb(node_t id) {

    if (next != NULL) {
        next->deleteNb(id);
    }
    uint8_t it = nbs.find(id);
    if (it != nbs.end()) {
        nbs.erase(it);
    }
}

void DHRoutingLayer::receive(DataIndication *ind, node_t ownId) {
    uint8_t dom;
    uint8_t length;
    node_t node;
    uint8_t hops;

    // debugging
//    if (ownId==0) {

//   }

    if (next == NULL) {
        return;
    }

    if (ind->src == ownId) {
        // TODO CAN THIS HAPPEN UNDER LEGAL CONDITIONS
        return;
    }

    ind->getAirframe() >> dom >> length;

    if (dom) {

        // only add node for which already a gateway exists
        if (BROADCAST != getGateway(ind->src)) {
            next->updateNb(ind->src, ind->src, 1);
        }
    } else {
        next->deleteNb(ind->src);
    }

    while (length > 0) {
        length--;
        ind->getAirframe() >> hops >> node;

        if (node == ownId) {
            continue;
        }
        if (hops < nbRadius) {

            // only add node for which already a gateway exists
            if (BROADCAST != getGateway(ind->src)) {
                next->updateNb(node, ind->src, hops + 1);
            }
        }
    }

}

node_t DHRoutingLayer::getNeighbor(uint8_t index) {

    uint8_t it = nbs.begin();

    while (it != nbs.end() && index > 0) {
        it = nbs.next(it);
        index--;
    }

    if (it != nbs.end()) {
        return nbs[it].id;
    }

    return BROADCAST ;

}

node_t DHRoutingLayer::getGateway() {
    if (nbs.size() == 0) {
        return BROADCAST ;
    }
    return nbs[nbs.begin()].id;
}

node_t DHRoutingLayer::getGateway(node_t id) {
    uint8_t it = nbs.find(id);

    // no entry found, return invalid address
    if (it == nbs.end()) {
        return BROADCAST ;
    }
    return nbs[it].next;
}

bool DHRoutingLayer::hasNb(node_t id) {
    return (nbs.end() != nbs.find(id));
}

void DHRoutingLayer::updateNb(node_t id, node_t next, uint8_t hops) {

    uint8_t it = nbs.find(id);
    if (it == nbs.end()) {
        DHRoutingNeighbor nb;
        nb.id = id;
        nb.next = next;
        nb.hops = hops;
        it = nbs.push_back(nb);
    }

    // insertion failed, maximum nb size reached
    if (it == nbs.end()) {
        return;
    }

    if (nbs[it].hops >= hops) {
        nbs[it].next = next;
        nbs[it].hops = hops;
        nbs[it].ttl = DHROUTING_MAX_TTL;
    }
}

void DHRoutingLayer::updateNbs() {

    uint8_t it = nbs.begin();
    while (it != nbs.end()) {

        if (nbs[it].ttl == 0) {
            it = nbs.erase(it);
        } else {
            nbs[it].ttl--;
            it = nbs.next(it);
        }

    }
}

void DHRoutingLayer::printNbs() {
    for (uint8_t it = nbs.begin(); it != nbs.end(); it = nbs.next(it)) {
        getCout() << "(" << nbs[it].next << "->" << nbs[it].id << ") ";
    }
}

DHRouting::DHRouting() :
        discoveryIndIn(this, &DHRouting::handleDiscoveryIndication,
                "discoveryIndIn"), discoveryReqOut(this, "discoveryReqOut"), nbUpdateIndIn(
                this, &DHRouting::handleUpdateIndication, "nbUpdateIndIn"), nbUpdateReqOut(
                this, "nbUpdateReqOut"), nbDiscover(true) {
    begin = new DHRoutingLayer(NB_RADIUS, MIS_RADIUS);
    begin->next = new DHRoutingLayer(NB_RADIUS, MIS_RADIUS);

}

DHRouting::~DHRouting() {
    delete begin;
}

void DHRouting::initialize() {
    Layer::initialize();

    counter = 1;

    maxDiscoveryTime = par("maxDiscoveryTime");

#ifdef ROUTING_ENABLE_STATS
    forwarded=0;
    control=0;
    numControlRecv=0;
#endif

    schedule(new Message, &DHRouting::timeout,
            getId() * DHROUTING_BASE_TIME + intrand(DHROUTING_BASE_TIME));

}

uint8_t DHRouting::getNextExecLayer() {

    uint8_t level = 0;
    uint16_t c = counter;
    while ((c & 1) == 0) {
        level++;
        c = c >> 1;
    }

    counter++;
    return level;
}

void DHRouting::finish() {
    Layer::finish();

    DHRoutingLayer* it = begin;
    uint8_t i = 0;
    getCout() << endl << getId() << " nbs: " << endl;

    while (it != NULL) {
        getCout() << "   level " << (int) i << ":";
        it->printNbs();
        getCout() << endl;
        it = it->next;
        i++;
    }

#ifdef ROUTING_ENABLE_STATS
    recordScalar("forwarded", forwarded);
    recordScalar("control",control);
    recordScalar("numControlRecv",numControlRecv);

#endif

}

void DHRouting::timeout(Message *timer) {

    // stop clustering algorithm
    if ((omnetpp::simTime().dbl() > maxDiscoveryTime) && (maxDiscoveryTime > 0)) {
        delete timer;
        return;
    }

    //COUT_AFTER_WARMUP("CONTINUE");

    schedule(timer, &DHRouting::timeout, DHROUTING_BASE_TIME);

    //begin->updateNbs();

    // send nb discover, with given frequency
    if (nbDiscover && (((counter - 1) % 4) == 0)) {

#ifdef ROUTING_ENABLE_STATS
        control++;
#endif
        discoveryReqOut.send(new DataRequest(BROADCAST, make_checked<Airframe>()),
                intrand(DHROUTING_BASE_TIME));

    }

    // send path update to sink
    // Path update to sink deactivated
    /*if ((simTime().dbl() > 40) && (getId() > 0) && ((counter % 30) == 0)) {
     uint8_t l = 0;
     node_t dom = getDominator(l);
     if (dom != BROADCAST ) {
     AirframePtr data = make_checked<Airframe>();
     (*data) << getId() << (uint8_t) 1 << (uint8_t) MSG_ROUTE_UPDATE
     << getId();
     DataRequest* req2 = new DataRequest(dom, data);
     routeMsg(req2, l, 0, intrand(DHROUTING_BASE_TIME));
     }
     }*/

    uint8_t level = getNextExecLayer();

    // send control data
    DHRoutingLayer* layer = getLayer(level);

    if (layer == NULL) {
        return;
    }

    DataRequest* req;
    if (level == 0) {
        req = layer->run(true, getId(), level);
    } else {
        DHRoutingLayer* layer2 = getLayer(level - 1);
        req = layer->run(layer2->nbs.size() > 0, getId(), level);
    }

    if (req == NULL) {
        return;
    }
    req->getAirframe() << getId();

    if (addRoutingInformation(req, level, 0)) {
        req->getAirframe() << (uint8_t) (0);  // add hop count value

#ifdef ROUTING_ENABLE_STATS
        control++;
#endif
        nbUpdateReqOut.send(req, intrand(DHROUTING_BASE_TIME));
    }

}

DataRequest * DHRouting::addRoutingInformation(DataRequest *req, bool force) {

    // searches for lowest possible HR address to destination

    DHRoutingLayer* layer = begin;

    uint8_t level = 0;

    while (layer != NULL) {
        if (layer->getGateway(req->dst) != BROADCAST ) {
            break;
        }
        level++;
        layer = layer->next;
    }

    if (layer != NULL) {
        //getCout() << "SEND " << getId() << " send to " << req->dst << " on level "
        //        << (uint16_t) level << endl;
        return addRoutingInformation(req, level, 0);
    } else {
        if (force) {
            DHRoutingLayer* layer = begin;
            level = 0;
            while (layer->next != NULL
                    && layer->next->getGateway() != BROADCAST ) {
                layer = layer->next;
                level++;
            }

            //ASSERT(layer->getGateway() !=BROADCAST);
            // TODO HERE IS SOME ERROR (or strange state), which is currently not fixed
            if (layer->getGateway() == BROADCAST ) {
                req->response(new DataResponse(DataResponseStatus::FAIL_UNKNOWN));
                delete req;
                return NULL;
            }

            //getCout()<<"FORCE " <<getId()<<" TO "<< layer->getGateway()<<endl;

            req->dst = layer->getGateway();
            return addRoutingInformation(req, level, 0);

        } else {
            req->response(new DataResponse(DataResponseStatus::FAIL_UNKNOWN));
            delete req;
            return NULL;
        }

    }

}

DataRequest * DHRouting::addRoutingInformation(DataRequest *req, uint8_t level,
        uint8_t level_offset) {

    if (level > 0) {
        DHRoutingLayer* layer = getLayer(level);

        // can not add routing information for the required level
        if (layer == NULL) {
            delete req;
            return NULL;
        }

        // find forwarder for data
        node_t dst = layer->getGateway(req->dst);
        //COUT_AFTER_WARMUP("find gateway "<<dst<<" for "<< req->dst);

        // if no forwarder is found delete packet
        if (dst == BROADCAST ) {
            delete req;
            return NULL;
        }

        // add path to packet
        req->getAirframe() << req->dst;
        req->dst = dst;
        return addRoutingInformation(req, level - 1, level_offset + 1);
    } else {
        // write length of path
        req->getAirframe() << level_offset;
        return req;
    }
}

DHRoutingLayer* DHRouting::getLayer(uint8_t level) {
    DHRoutingLayer* layer = begin;
    while (level != 0 && layer != NULL) {
        layer = layer->next;
        level--;
    }
    return layer;
}

void DHRouting::receiveRouteUpdate(DataIndication *ind) {

    /*
     uint8_t length, l;
     ind->getAirframe() >> length;

     if (getId() > 0) {
     length++;
     ind->getAirframe() << getId() << length << (uint8_t) MSG_ROUTE_UPDATE
     << getId();
     node_t dom = getDominator(l);
     if (dom != BROADCAST ) {
     routeMsg(new DataRequest(dom, ind->decapsulateAirframe()), l, 0, 0);
     }
     } else {

     node_t node;

     std::list<node_t> path;

     ASSERT(length>0);
     while (length) {
     length--;
     ind->getAirframe() >> node;
     path.push_front(node);
     }
     paths[path.front()] = path;

     }
     */
    delete ind;

}

node_t DHRouting::getDominator(uint8_t &level) {
    level = 0;
    DHRoutingLayer* layer = begin;
    while (layer->next != NULL) {
        layer = layer->next;
        level++;
    }

    return layer->getLowestNbId();

}

void DHRouting::handleDiscoveryIndication(DataIndication* msg) {
    //COUT_AFTER_WARMUP("DISCOVERY");

#ifdef ROUTING_ENABLE_STATS
    numControlRecv++;
#endif

    MacRxInfo* phy = msg->get<MacRxInfo>();
    if (phy->lqi < LQI_FILTER && !begin->hasNb(msg->src)) {
        delete msg;
        return;
    }

    begin->updateNb(msg->src, msg->src, 1);
    delete msg;
}

void DHRouting::handleUpdateIndication(DataIndication* msg) {

#ifdef ROUTING_ENABLE_STATS
    numControlRecv++;
#endif

    // also update neighbor list
    //begin->updateNb(msg->src, msg->src, 1);

    uint8_t addrLevel;
    uint8_t hops;

    // get length of hr address
    msg->getAirframe() >> hops >> addrLevel;

    uint8_t level = 0;
    node_t dst = msg->src;

    // extract all occurrences of this node from address
    while (addrLevel > 0) {
        level++;
        msg->getAirframe() >> dst;
        msg->dst = dst;
        if (dst != getId()) {
            break;
        }
        addrLevel--;
    }

    // target is reached
    if (addrLevel == 0) {
        // get identifier of source node
        msg->getAirframe() >> msg->src;
        DHRoutingLayer* layer = getLayer(level);
        if (layer != NULL) {
            layer->receive(msg, getId());
        }
    } else if (hops < DHROUTING_MAX_HOPS) {
        // forward packet
        DataRequest * req = new DataRequest(msg->dst,
                msg->decapsulateAirframe());
        if (addRoutingInformation(req, level, addrLevel - 1)) {
            hops++;
            req->getAirframe() << hops;

#ifdef ROUTING_ENABLE_STATS
            control++;
#endif

            nbUpdateReqOut.send(req, intrand(DHROUTING_BASE_TIME));
        }

    }

    delete msg;
}

void DHRouting::handleIndication(DataIndication* msg) {

    //getCout() << "RECEIVE " << getId() << endl;
    uint8_t addrLevel;
    uint8_t hops;

    // get length of hr address
    msg->getAirframe() >> hops >> addrLevel;

    uint8_t level = 0;
    node_t dst = msg->src;

    // extract all occurrences of this node from address
    while (addrLevel > 0) {
        level++;
        msg->getAirframe() >> dst;
        msg->dst = dst;
        if (dst != getId()) {
            break;
        }
        addrLevel--;
    }

    // target is reached
    if (addrLevel == 0) {
        node_t dst;
        msg->getAirframe() >> dst >> msg->src;

        if (dst != getId()) {
            //getCout()<<"FORWARD " <<getId()<<" TO "<< dst<<endl;
            handleRequest(new DataRequest(dst, msg->decapsulateAirframe()),
                    msg->src);
            delete msg;
        } else {
            // send to upper layer
            msg->set(new RoutingInfo(hops + 1));
            Layer::sendIndication(msg);
        }

    } else if (hops < DHROUTING_MAX_HOPS) {
        // forward packet

        DataRequest * req = new DataRequest(msg->dst,
                msg->decapsulateAirframe());
        if (addRoutingInformation(req, level, addrLevel - 1)) {
            //getCout() << "FORWARD " << endl;
            hops++;
            req->getAirframe() << hops;

#ifdef ROUTING_ENABLE_STATS
            forwarded++;
#endif
            Layer::sendRequest(req);
        }
        delete msg;
    } else {
        delete msg;
    }

}

/*
 void DHRouting::receiveData(DataIndication *ind) {

 uint8_t addrLevel;

 // get length of hr address
 //msg->getAirframe() >> addrLevel;

 uint8_t length;
 ind->getAirframe() >> length;
 if (length == 0) {
 ind->src = 0;
 ind->dst = getId();
 send(ind); // send to upper layer
 } else {
 node_t next;
 ind->getAirframe() >> next;
 length--;
 ind->getAirframe() << length << (uint8_t) MSG_DATA << getId();
 routeMsg(new DataRequest(next, ind->decapsulateAirframe()), 0);

 delete ind;
 }
 }*/

void DHRouting::handleRequest(DataRequest* msg, node_t source) {
    ASSERT(msg->dst==0);
    // currently only routing data to sink is support

    // try to append path information to packet
    msg->getAirframe() << source << msg->dst; // add sender and destination address

    msg = addRoutingInformation(msg, true);
    if (msg) {
        //getCout() << "SEND << " << getId() << " TO " << msg->dst << endl;
        msg->getAirframe() << (uint8_t) 0; // hops

#ifdef ROUTING_ENABLE_STATS
        forwarded++;
#endif
        Layer::sendRequest(msg);
    }
}

void DHRouting::handleRequest(DataRequest* msg) {
    handleRequest(msg, getId());
}

} // namespace cometos
