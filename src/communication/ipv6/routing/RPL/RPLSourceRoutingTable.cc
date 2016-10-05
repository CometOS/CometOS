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
 * @author Fabian Krome, Martin Ringwelski, Andreas Weigel
 */

#include "RPLSourceRoutingTable.h"
#include "RPLRouting.h"


namespace cometos_v6 {

Define_Module(RPLSourceRoutingTable);


bool RPLSourceRoutingTable::isRouter() const {
    return true;
}

void RPLSourceRoutingTable::initialize() {
    it = (IPv6InterfaceTable*) getModule(INTERFACE_TABLE_MODULE_NAME);
    ASSERT(it != NULL);

    //FABIAN
    defaultRoute = NULL;

}

const IPv6Route* RPLSourceRoutingTable::doLongestPrefixMatch(const IPv6Address & dest) {
    uint8_t route = 0xFF;
    uint8_t prefixLength = 0;
    LOG_DEBUG("Srch fr Mtch to " << dest.str());
    for (uint8_t i=routeList.begin(); i != routeList.end(); i = routeList.next(i))
    {
        // TODO: Also do some checking, if the entries are expired
        ASSERT(i != routeList.next(i));

        bool prefixIsLonger = prefixLength < routeList[i]->getPrefixLength();
        const IPv6Address & pre = routeList[i]->getDestPrefix();
        uint8_t len = routeList[i]->getPrefixLength();
        bool destMatches = dest.matches(pre, len);
        if ( prefixIsLonger && destMatches)
        {
            LOG_DEBUG("Fnd Match dest: " << dest.str() << " found: " << routeList[i]->getDestPrefix().str());
            prefixLength = routeList[i]->getPrefixLength();
            LOG_DEBUG("Fnd Match length: " << (int)prefixLength);
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
        LOG_DEBUG("no match - nexthop up: " << defaultRoute->getNextHop().str());
    } else {
        LOG_DEBUG("no default route");
    }
    return defaultRoute;
}


route_key_t RPLSourceRoutingTable::addRoute(const IPv6Address * destPrefix,
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


route_key_t RPLSourceRoutingTable::modifyRoute(const IPv6Address * destPrefix,
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

route_key_t RPLSourceRoutingTable::internAddRoute(IPv6Route * route) {
    // TODO routes could and should be sorted in a future implementation
    route_key_t rk = routeList.push_front(route);
    if (rk == routeList.end()) {
        delete route;
        return 0xFF;
    } else {
        LOG_DEBUG("Ins rt " << route->getNextHop().getAddressPart(7));
    }
    return rk;
}

//FABIAN KROME

void RPLSourceRoutingTable::updateDefaultRoute(const IPv6Address * nextHop,
        uint16_t interfaceId,
        int metric){

    deleteDefaultRoute();

    //default route mit "0-Route" l�nge 0 - kein prefix, length auto 0,

    defaultRoute = new IPv6Route(
            *nextHop,
            16 * 8,
            IPv6Route::STATIC,
            0,
            1);
    defaultRoute->setNextHop(nextHop);
}

bool RPLSourceRoutingTable::deleteRoute(IPv6Address ip, DAO_Object DAO_info){


    bool deleted = false;
    uint8_t index = routeList.begin();

    if(index == routeList.end()){
        return false;
    }

    while (index != routeList.end()){
        IPv6Route *route = routeList.get(index);

        if((route->getPrefixLength() == 128 && route->getDestPrefix() == ip) ||
                route->getNextHop() == ip)
        {
            //DAO_info.deleteTarget(DAO_info.findTarget(route->getDestPrefix()));
            LOG_DEBUG("route " << route->getDestPrefix().str() << "/" << (int)(route->getPrefixLength()) << " over " << route->getNextHop().str());
            uint8_t newIndex = routeList.next(index);
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
