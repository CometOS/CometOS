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
 * @author Andreas Weigel
 */

#include "RoutingTable.h"
#ifdef COMETOS_V6_RPL
#include "RPLRouting.h"
#endif

namespace cometos_v6 {

const uint8_t RoutingTable::RT_ROUTING_TABLE_SIZE;

Define_Module(RoutingTable);

const char *IPv6Route::routeSrcName(RouteSrc src)
{
    switch (src)
    {
        case FROM_RA:         return "FROM_RA";
        case OWN_ADV_PREFIX:  return "OWN_ADV_PREFIX";
        case STATIC:          return "STATIC";
        case ROUTING_PROT:    return "ROUTING_PROT";
        default:              return "???";
    }
}

void RoutingTable::clearRouteList() {
    //LOG_DEBUG("Delete all Routes");
    for (uint8_t i=routeList.begin(); i != routeList.end(); i = routeList.next(i)) {
        delete routeList.get(i);
    }
    routeList.clear();
}

bool RoutingTable::isRouter() const {
    return true;
}

void RoutingTable::initialize() {
    it = (IPv6InterfaceTable*) getModule(INTERFACE_TABLE_MODULE_NAME);
    ASSERT(it != NULL);
//    uint16 tmp[8] = {0x64,29,30,31,32,33,0,0};
//    // create a fixed Route entry depending on this node's ID
//    IPv6Address addr(tmp);
//    IPv6Route * route = new IPv6Route(addr, 96, IPv6Route::STATIC);
//
//    // TODO for now, set next hop by exploiting a chain with descending IDs
//    // simply append own ID - 1 to last segment of nextHop address
//    addr.setAddressPart(getId()-1, 7);
//    route->setNextHop(addr);
//    route->setInterfaceId(0);
//    route->setExpiryTime(0);
//    routeList.push_front(route);
}


bool RoutingTable::isLocalAddress(const IPv6Address & address) const {
    LOG_DEBUG("Chk if " << address.str() << " lcl Addr");
    for (uint8_t i = 0; i < it->getNumInterfaces(); i++) {
        for (uint8_t j = 0; j < it->getInterface(i).getNumAddresses(); j++) {
            LOG_DEBUG("Interface Address: " << it->getInterface(i).getAddress(j).str());
            if (it->getInterface(i).getAddress(j) == address) {
                LOG_DEBUG("Is local");
                return true;
            }
        }
    }
    if (    address.isLoopback() ||
            address.isLoopbackBroadcast() ||
            address.isLocalBroadcast()
            ) {
        return true;
    }
    if (isRouter() && (
            address.isLoopbackRouterBroadcast() ||
            address.isLocalRouterBroadcast() ||
            address.isSiteRouterBroadcast())) {
        return true;
    }

    //FABIAN
    //RPL multicast address with a link-local scope for RPL nodes called all-RPL-nodes
    if (
                address.getAddressPart(0) == 0xff02 &&
                address.getAddressPart(1) == 0 &&
                address.getAddressPart(2) == 0 &&
                address.getAddressPart(3) == 0 &&
                address.getAddressPart(4) == 0 &&
                address.getAddressPart(5) == 0 &&
                address.getAddressPart(6) == 0 &&
                address.getAddressPart(7) == 0x1a
                )
     {
        return true;
    }

    return false;
}

const IPv6Route* RoutingTable::doLongestPrefixMatch(const IPv6Address & dest) {
    uint8_t route = 0xFF;
    int16_t prefixLength = -1;
    LOG_DEBUG("Srch fr Mtch " << dest.str());
    for (uint8_t i=routeList.begin(); i != routeList.end(); i = routeList.next(i))
    {
        // TODO: Also do some checking, if the entries are expired
        ASSERT(i != routeList.next(i));

        LOG_DEBUG(routeList[i]->getDestPrefix().str() << "/" << (int)routeList[i]->getPrefixLength() << " over " << routeList[i]->getNextHop().str());

        bool prefixIsLonger = prefixLength < routeList[i]->getPrefixLength();
        const IPv6Address & pre = routeList[i]->getDestPrefix();
        uint8_t len = routeList[i]->getPrefixLength();
        bool destMatches = dest.matches(pre, len);
        if ( prefixIsLonger && destMatches)
        {
            prefixLength = routeList[i]->getPrefixLength();
            LOG_DEBUG("Fnd Prefix Match: " << routeList[i]->getDestPrefix().str() << "/" << (int)prefixLength);
            if (prefixLength == 128) {
                LOG_DEBUG("Perfect Match, skip rest");
                return (routeList[i]);
            }
            route = i;
        }
    }

    LOG_DEBUG("Done");
    if (route < 0xFF) {
        LOG_DEBUG("found match");
        return (routeList[route]);

    }
    //FABIAN
    //return NULL;
    //Return possible default Route -> create new Route for prefix
    if(defaultRoute != NULL){
        LOG_INFO("default rt: " << cometos::hex << defaultRoute->getNextHop().getAddressPart(7) << cometos::dec);
    } else {
        LOG_WARN("no default rt");
#ifdef COMETOS_V6_RPL
        // FIXME: Workaround, because sometimes the default route gets overwritten. There seems to be a memory leak somewhere...
        RPLRouting* rm = static_cast<RPLRouting *>(getModule(RPL_MODULE_NAME));
        rm->sendDIS();
#endif
    }
    return defaultRoute;
}


bool RoutingTable::getActiveStateForNextHop(const IPv6Address& nextHop) {
    if (defaultRoute != NULL && defaultRoute->getNextHop() == nextHop) {
        if (defaultRoute->isActive()) {
            return true;
        }
    }
    for(uint8_t idx = routeList.begin(); idx != routeList.end(); idx = routeList.next(idx)) {
        LOG_DEBUG("curr=" << routeList[idx]->getNextHop().str() << "|sought=" << nextHop.str())
        if (routeList[idx]->getNextHop() == nextHop) {
            LOG_DEBUG("Found; isActive()=" << (int) routeList[idx]->isActive());
            if (routeList[idx]->isActive()) {
                // if we found a route with active flag set for the given next
                // hop, return true
                return true;
            }
        }
    }

    // no route with the given next hop and active flag set was found
    return false;
}

void RoutingTable::setActiveStateForNextHop(const IPv6Address& nextHop, bool value) {
    if (defaultRoute != NULL && defaultRoute->getNextHop() == nextHop) {
        defaultRoute->setActive(value);
    }
    for(uint8_t idx = routeList.begin(); idx != routeList.end(); idx = routeList.next(idx)) {
        LOG_DEBUG("curr=" << routeList[idx]->getNextHop().str() << "|sought=" << nextHop.str())
        if (routeList[idx]->getNextHop() == nextHop) {
            LOG_DEBUG("Found; set to " << (int) value);
            // set found nextHop entry to active
            routeList[idx]->setActive(value);
        }
    }
}

route_key_t RoutingTable::addRoute(const IPv6Address * destPrefix,
                            uint8_t prefixLen,
                            const IPv6Address * nextHop,
                            uint16_t interfaceId,
                            int metric)
{
    if (metric==0)
        metric = 10; // TBD should be filled from interface metric

    // create route object
    IPv6Route *route = new IPv6Route(
            *destPrefix,
            prefixLen,
            IPv6Route::STATIC,
            interfaceId,
            metric);

    route->setNextHop(nextHop);

    // then add it
    return internAddRoute(route);
}


route_key_t RoutingTable::modifyRoute(const IPv6Address * destPrefix,
                            uint8_t prefixLen,
                            const IPv6Address * nextHop,
                            uint16_t interfaceId,
                            int metric)
{

    uint8_t index = routeList.begin();

    IPv6Route *route = routeList.get(index);

    while(index != routeList.end()){
        LOG_DEBUG("route" << route->getDestPrefix().str()<<"\n");

        route = routeList.get(index);

        if(route->getDestPrefix() == *destPrefix){
            routeList.erase(index);
            index = routeList.end();
        } else {
            index = routeList.next(index);
        }
    }

    if (metric==0)
        metric = 10; // TBD should be filled from interface metric

    // create route object
    IPv6Route *route_new = new IPv6Route(
            *destPrefix,
            prefixLen,
            IPv6Route::STATIC,
            interfaceId,
            metric);

    route_new->setNextHop(nextHop);

    // then add it
    return internAddRoute(route_new);
}

route_key_t RoutingTable::internAddRoute(IPv6Route * route) {
    // TODO routes could and should be sorted in a future implementation
    if (route->getPrefixLength() == 128 && isLocalAddress(route->getDestPrefix())) {
        LOG_INFO("Do not add Route to self");
        delete route;
        return 0xFF;
    }
    route_key_t rk = routeList.push_front(route);
    if (rk == routeList.end()) {
        delete route;
        return 0xFF;
    } else {
        LOG_DEBUG("Ins rt " << route->getDestPrefix().str() << "/" << (int)(route->getPrefixLength()) << " over " << route->getNextHop().str());
    }
    return rk;
}

//FABIAN KROME

void RoutingTable::updateDefaultRoute(const IPv6Address * nextHop,
        uint16_t interfaceId,
        int metric){

    deleteDefaultRoute();

    LOG_INFO("Update default rt");

    //default route mit "0-Route" laenge 0 - kein prefix, length auto 0,

    defaultRoute = new IPv6Route(
            IPv6Address(),
            0,
            IPv6Route::STATIC,
            interfaceId,
            metric);
    defaultRoute->setNextHop(nextHop);
}


void RoutingTable::deleteDefaultRoute(){
    if (defaultRoute != NULL) {
        LOG_WARN("Del deflt Rt");
        delete defaultRoute;
        defaultRoute = NULL;
    }
}

bool RoutingTable::deleteRoute(IPv6Address ip) { //, DAO_Object DAO_info){

    LOG_DEBUG("Deleting Route for " << ip.str());

    bool deleted = false;
    uint8_t index = routeList.begin();

    if(index == routeList.end()){
        return false;
    }

    IPv6Route *route = routeList.get(index);
    uint8_t newIndex;


    while(index != routeList.end()){
        LOG_DEBUG("route" << route->getDestPrefix().str()<<"\n");

        route = routeList.get(index);

        if(route->getDestPrefix() == ip){
            //DAO_info.deleteTarget(DAO_info.findTarget(route->getDestPrefix()));
            newIndex = routeList.next(index);
            routeList.erase(index);
            index = newIndex;
            deleted = true;
            continue;
        }

        if(route->getNextHop() == ip){
            //DAO_info.deleteTarget(DAO_info.findTarget(route->getDestPrefix()));
            newIndex = routeList.next(index);
            routeList.erase(index);
            index = newIndex;
            deleted = true;
            continue;
        }
        index = routeList.next(index);
    }

    return deleted;

}

}
