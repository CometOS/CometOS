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

#include "RPLRoutingTable.h"
#include "RPLRouting.h"

namespace cometos_v6 {

Define_Module(RPLRoutingTable);


bool RPLRoutingTable::isRouter() const {
    return true;
}

route_key_t RPLRoutingTable::addRoute(const IPv6Address * destPrefix,
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
            IPv6Route::ROUTING_PROT,
            interfaceId,
            metric);

    route->setNextHop(nextHop);

    // then add it
    return internAddRoute(route);
}


route_key_t RPLRoutingTable::modifyRoute(const IPv6Address * destPrefix,
                            uint8_t prefixLen,
                            const IPv6Address * nextHop,
                            uint16_t interfaceId,
                            int metric)
{

    uint8_t index = routeList.begin();


    IPv6Route *route = routeList.get(index);

    while(index != routeList.end()){
        LOG_DEBUG("route" << route->getDestPrefix().str());

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
            IPv6Route::ROUTING_PROT,
            interfaceId,
            metric);

    route_new->setNextHop(nextHop);

    // then add it
    return internAddRoute(route_new);
}

route_key_t RPLRoutingTable::internAddRoute(IPv6Route * route) {
    // TODO routes could and should be sorted in a future implementation
    //cometos::getCout()<<"adding Route to RT"<<cometos::endl;
    route_key_t rk = routeList.push_front(route);
    if (rk == routeList.end()) {
        delete route;
        return 0xFF;
    } else {
        LOG_DEBUG("Ins rt " << route->getNextHop().getAddressPart(7));
    }
    return rk;
}

void RPLRoutingTable::printAllRoutes() {
    cometos::getCout()<< "trying to print all routes, routeList size == "<< (int) routeList.size()<<cometos::endl;
    if(routeList.size() > 0) {
        for(uint8_t i = 0; i<routeList.size(); i++) {
            IPv6Route* route = routeList.get(i);
            cometos::getCout()<<"Route: "<< (int) i<<" | next Hop: "<<route->getNextHop().str().c_str()<<" | dst: " << route->getDestPrefix().str().c_str()<<cometos::endl;
        }
    }
    return;
}

//FABIAN KROME

void RPLRoutingTable::updateDefaultRoute(const IPv6Address * nextHop,
        uint16_t interfaceId,
        int metric){

    LOG_WARN("Update Default Rt");

    deleteDefaultRoute();

    //default route mit "0-Route" l�nge 0 - kein prefix, length auto 0,

    defaultRoute = new IPv6Route(
            IPv6Address(),
            0,
            IPv6Route::ROUTING_PROT,
            interfaceId,
            metric);
    defaultRoute->setNextHop(nextHop);
}

bool RPLRoutingTable::deleteRoute(IPv6Address ip, DAO_Object DAO_info){


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
            LOG_WARN("Del route " << cometos::hex << route->getDestPrefix().getAddressPart(7) << cometos::dec
                    << "/" << (int)(route->getPrefixLength())
                    << " over " << cometos::hex << route->getNextHop().getAddressPart(7) << cometos::dec);
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
