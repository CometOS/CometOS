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

#ifndef __COMETOS_V6_RPLROUTINGTABLE_H_
#define __COMETOS_V6_RPLROUTINGTABLE_H_

#include "Module.h"
#include "RPLBasics.h"
#include "RoutingTable.h"

namespace cometos_v6 {

#define RPL_ROUTING_TABLE_MODULE_NAME "rt"

/**
 *
 */
class RPLRoutingTable : public RoutingTable {
public:

    RPLRoutingTable(const char * service_name = NULL): RoutingTable(service_name) {}

    /**
     * IP forwarding on/off
     */
    virtual bool isRouter() const;

    /**
     * Creates a static route. If metric is omitted, it gets initialized
     * to the interface's metric value.
     */
    virtual route_key_t addRoute(const IPv6Address* destPrefix, uint8_t prefixLength,
                        const IPv6Address* nextHop, uint16_t interfaceId,
                        int metric = 0);

//    virtual IPv6Address & getNextHop(const IPv6Address &address);

    //FABIAN
    virtual route_key_t modifyRoute(const IPv6Address* destPrefix, uint8_t prefixLength,
            const IPv6Address* nextHop, uint16_t interfaceId,
            int metric = 0);

    bool deleteRoute(IPv6Address ip, DAO_Object DAO_info);

    virtual void updateDefaultRoute(const IPv6Address * destPrefix,
            uint16_t interfaceId,
            int metric);

    const IPv6Route* getDefaultRoute() const { return defaultRoute;}

    void printAllRoutes();

protected:
    route_key_t internAddRoute(IPv6Route * route);

};

}

#endif



///**
// * Insert a forwarding-table mapping for the given prefix, with the
// * given next-hop.
// */
//command route_key_t addRoute(const uint8_t *prefix, int prefix_len_bits,
//                             struct in6_addr *next_hop, uint8_t ifindex);
//
///**
// * Remove a routing table entry previously inserted using addRoute
// */
//command error_t delRoute(route_key_t key);
//
//command struct route_entry *lookupRoute(const uint8_t *prefix, int prefix_len_bits);
//
//command struct route_entry *lookupRouteKey(route_key_t key);
//
//command struct route_entry *getTable(int *size);
