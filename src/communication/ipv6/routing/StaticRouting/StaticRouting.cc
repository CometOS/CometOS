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

#include "StaticRouting.h"

#ifdef OMNETPP
#include "XMLParseUtil.h"
#endif
#ifdef PAL_PERS
#include "palPers.h"
#define PERS_ROUTING_TABLE_ADDR_START 0x800
#define PERS_ROUTING_TABLE_IPADDR_SIZE 16
#define PERS_ROUTING_TABLE_ENTRY_SIZE 34
#define PERS_ROUTING_TABLE_ADDR_SIZE (PERS_ROUTING_TABLE_ADDR_START + PERS_ROUTING_TABLE_ENTRY_SIZE - 1)
#endif

namespace cometos_v6 {

Define_Module(StaticRouting);

void StaticRouting::initialize() {
    if (rt == NULL) rt = (RoutingTable *) getModule(ROUTING_TABLE_MODULE_NAME);
    ASSERT(rt != NULL);

    remoteDeclare(&StaticRouting::addRoute, "add");
    remoteDeclare(&StaticRouting::clearRouting, "clear");
#ifdef PAL_PERS
    remoteDeclare(&StaticRouting::storeRoute, "sr");
    remoteDeclare(&StaticRouting::clearStore, "cs");
    remoteDeclare(&StaticRouting::getRoute, "gr");
    remoteDeclare(&StaticRouting::getNumStored, "gnr");
#endif

#ifdef OMNETPP
    // open route configuration file to get static routes
    omnetpp::cXMLElement* routingFile;
    CONFIG_NED(routingFile);
    omnetpp::cXMLElementList nodes = routingFile->getElementsByTagName("node");

    for (omnetpp::cXMLElementList::iterator itNodes = nodes.begin(); itNodes != nodes.end(); itNodes++) {
        node_t currNode = XMLParseUtil::getAddressFromAttribute(*itNodes, "id");
        if (getId() == currNode) {
            IPv6Address nextHop = (*itNodes)->getAttribute("nextHop");
            IPv6Address prefix = (*itNodes)->getAttribute("prefix");
            const char * tmp = (*itNodes)->getAttribute("prefixLength");
            ASSERT(tmp != NULL);
            uint8_t prefixLength = atoi(tmp);
            rt->addRoute(&prefix, prefixLength, &nextHop, 0);
            LOG_DEBUG("Routing: " << currNode << " --> " << prefix.str()
                    << "/" << (int)prefixLength << " over " << nextHop.str());
        }
    }
#endif
	// loadRoutes used to be called directly here, which led to an initializion race! DONT!
    schedule(new cometos::Message(), &StaticRouting::loadRoutes, 0);
}




#ifdef PAL_PERS
//TODO abstract this stuff into some generic persistent memory
bool StaticRouting::storeRoute(RouteInfo & route, uint8_t & pos) {
    uint8_t numEntries;
    palPers_read(PERS_ROUTING_TABLE_ADDR_SIZE, &numEntries, 1);
    if (pos >= RoutingTable::RT_ROUTING_TABLE_SIZE
            || pos > numEntries) {
        return false;
    }
    else {
        uint16_t start = PERS_ROUTING_TABLE_ADDR_START + (PERS_ROUTING_TABLE_ENTRY_SIZE * pos);
        storeAddr(route.prefix, start);
        storeAddr(route.nextHop, start + PERS_ROUTING_TABLE_IPADDR_SIZE);
        palPers_write(start + PERS_ROUTING_TABLE_IPADDR_SIZE * 2, &route.prefixLength, 1);
        if (pos == numEntries) {
            numEntries++;
            palPers_write(PERS_ROUTING_TABLE_ADDR_SIZE, &numEntries, 1);
        }
        return true;
    }
}

void StaticRouting::storeAddr(IPv6Address const & ipa, uint16_t persAddr) {
    uint8_t a[16];
    for (uint8_t i =0; i < 8; i++) {
        a[i*2] = (uint8_t) (ipa.getAddressPart(i) >> 8);
        a[i*2+1] = (uint8_t) (ipa.getAddressPart(i) & 0xFF);
    }
    palPers_write(persAddr, a, 16);
}

void StaticRouting::loadAddr(IPv6Address & ipa, uint16_t persAddr) {
    uint8_t a[16];
    palPers_read(persAddr, a, 16);
    ipa.set(a);
}

uint8_t StaticRouting::getNumStored() {
    uint8_t numEntries;
    palPers_read(PERS_ROUTING_TABLE_ADDR_SIZE, &numEntries, 1);
    // if numEntries is larger than the routing table size, there is obviously
    // an inconsistency, e.g. an unitialized flash
    return numEntries <= RoutingTable::RT_ROUTING_TABLE_SIZE ? numEntries : 0;
}

RouteInfo StaticRouting::getRoute(uint8_t & pos) {
    RouteInfo ri;
    if (pos >= getNumStored()) {
        return ri;
    } else {
        uint16_t start = PERS_ROUTING_TABLE_ADDR_START + (PERS_ROUTING_TABLE_ENTRY_SIZE * pos);
        loadAddr(ri.prefix, start);
        loadAddr(ri.nextHop, start+16);
        palPers_read(start + 32, &(ri.prefixLength), 1);
        return ri;
    }
}

bool StaticRouting::clearStore() {
    uint8_t tmp = 0;
    palPers_write(PERS_ROUTING_TABLE_ADDR_SIZE, &tmp, 1);
    return true;
}

#endif

void StaticRouting::loadRoutes(cometos::Message * msg) {
    delete(msg);
#ifdef PAL_PERS
    uint8_t numEntries;
    rt->clearRouteList();
    palPers_read(PERS_ROUTING_TABLE_ADDR_SIZE, &numEntries, 1);
    for (uint8_t k = 0; k < numEntries && k < RoutingTable::RT_ROUTING_TABLE_SIZE; k++) {
        static RouteInfo ri;
        ri = getRoute(k);

        // we at least check for prefix length to identify obviously
        // inconsistent routes in persistent memory
        if (ri.prefixLength <= IPv6Address::MAX_PREFIX_LENGTH) {
            addRoute(ri);
        }
    }
#endif
}



void StaticRouting::addRoute(RouteInfo & route) {
    LOG_DEBUG("Setting Route. Prefix " << route.prefix.getAddressPart(7)
            << "/" << (uint16_t)route.prefixLength
            << " to " << route.nextHop.getAddressPart(7));
    rt->addRoute(&(route.prefix), route.prefixLength, &(route.nextHop), 0);
}

void StaticRouting::clearRouting() {
    LOG_DEBUG("Clearing Routing Info");
    rt->clearRouteList();
}

void StaticRouting::txResult(const IPv6Address & dst, const LlTxInfo & info) {
    LOG_INFO("txResult: " << (uint16_t) info.getNumRetries() << "|" << (uint16_t) info.getNumSuccessfulTransmissions());
}

void StaticRouting::rxResult(const IPv6Address & src, const LlRxInfo & info) {
    LOG_INFO("rxResult: " << (uint16_t) info.avgRssi() << "|" << (uint16_t) info.avgLqi() << "|" << info.isLqiValid());
}


}

namespace cometos {
void serialize(ByteVector& buffer, const RouteInfo & value) {
    serialize(buffer, value.prefix);
    serialize(buffer, value.prefixLength);
    serialize(buffer, value.nextHop);
}
void unserialize(ByteVector& buffer, RouteInfo & value) {
    unserialize(buffer, value.nextHop);
    unserialize(buffer, value.prefixLength);
    unserialize(buffer, value.prefix);
}
}
