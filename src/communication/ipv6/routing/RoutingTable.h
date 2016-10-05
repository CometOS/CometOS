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

#ifndef __COMETOS_V6_ROUTINGTABLE_H_
#define __COMETOS_V6_ROUTINGTABLE_H_

#include "Module.h"
#include "SList.h"
#include "IPv6Address.h"
#include "IPv6InterfaceTable.h"
#include "palLocalTime.h"

namespace cometos_v6 {

#define ROUTING_TABLE_MODULE_NAME "rt"

//FABIAN
const uint8_t NULL_PREFIX_LENGTH = 0;

typedef uint8_t route_key_t;

/**
 * Represents a route in the route table. Routes with src=FROM_RA represent
 * on-link prefixes advertised by routers.
 *
 * TODO this is taken from INET and most likely contains some overkill, to be revisited
 */
class IPv6Route
{
  public:
    /** Specifies where the route comes from */
#if defined SWIG || defined BOARD_python
    enum RouteSrc
#else
    enum RouteSrc : uint8_t
#endif
    {
        FROM_RA,        ///< on-link prefix, from Router Advertisement
        OWN_ADV_PREFIX, ///< on routers: on-link prefix that the router **itself** advertises on the link
        STATIC,         ///< static route
        ROUTING_PROT,   ///< route is managed by a routing protocol (OSPF,BGP,etc)
    };

  protected:
    IPv6Address _destPrefix;
    uint8_t _activeFlag:1;
    uint8_t _prefixLength:7;
    RouteSrc _src;
    uint8_t _interfaceID;      //XXX IPv4 IRoutingTable uses interface pointer
    IPv6Address _nextHop;  // unspecified means "direct"
    time_ms_t _expiryTime;
    uint16_t _metric;

  public:
    /**
     * Constructor. The destination prefix and the route source is passed
     * to the constructor and cannot be changed afterwards.
     *
     * TODO check which fields are really needed and if some might even have to be added
     */
    IPv6Route(const IPv6Address& destPrefix,
            uint8_t length,
            RouteSrc src,
            uint8_t interfaceID = 0,
            uint16_t metric = 0,
            bool active = false):
        _destPrefix(destPrefix),
        _activeFlag(active),
        _prefixLength(length-1),
        _src(src),
        _interfaceID(interfaceID),
        _expiryTime(0),
        _metric(metric)
    {
        // prefix length has to be a number in the range [1,128]
        // we subtract 1 to get it into a 7-bit part of an uint8_t
        // to get some space for an additional flag
        ASSERT(length > 0);
    }

//    virtual std::string info() const;
//    virtual std::string detailedInfo() const;
    static const char *routeSrcName(RouteSrc src);

    void setInterfaceId(uint8_t interfaceId)  {_interfaceID = interfaceId;}
    void setNextHop(const IPv6Address* nextHop)  {_nextHop = *nextHop;}
    void setExpiryTime(time_ms_t expiryTime)  {_expiryTime = expiryTime;}
    void setMetric(uint16_t metric)  {_metric = metric;}

    const IPv6Address& getDestPrefix() const {return _destPrefix;}
    uint8_t getPrefixLength() const  {return _prefixLength+1;}
    bool isActive() const {return _activeFlag;}
    void setActive(bool value) {_activeFlag = value;}
    RouteSrc getSrc() const  {return _src;}
    uint8_t getInterfaceId() const  {return _interfaceID;}
    const IPv6Address& getNextHop() const  {return _nextHop;}
//    simtime_t getExpiryTime() const  {return _expiryTime;}
    time_ms_t getExpiryTime() const {return _expiryTime;}
    uint16_t getMetric() const  {return _metric;}

};


/**
 * Generic class storing routing information, installed there
 * by some routing protocol, neighbor discovery
 *
 * TODO fixed size ok for hardware but not nice for powerful platforms
 */
class RoutingTable : public cometos::Module {
public:

#if defined OMNETPP or defined BOARD_python
    static const uint8_t RT_ROUTING_TABLE_SIZE = 100;
#elif defined BOARD_M3OpenNode or defined BOARD_A8_M3
    static const uint8_t RT_ROUTING_TABLE_SIZE = 35;
#else
    static const uint8_t RT_ROUTING_TABLE_SIZE = 10;
#endif

    typedef cometos::StaticSList<IPv6Route *, RT_ROUTING_TABLE_SIZE> RouteList;

    RoutingTable(const char * service_name = NULL):
        Module(service_name), it(NULL), defaultRoute(NULL) {}

    virtual ~RoutingTable() { clearRouteList(); deleteDefaultRoute(); }

    /**
     * IP forwarding on/off
     */
    virtual bool isRouter() const;

    virtual bool isLocalAddress(const IPv6Address& address) const;

    const IPv6Route* doLongestPrefixMatch(const IPv6Address & dest);


    /** Reverse lookup method, which retrieves an active flag
     * for the given next hop.
     *
     * @param[in] nextHop IPv6Address to be searched for
     * @return
     *      true, if one of the route entries that use nextHop as their next hop
     *            is marked as active (logically ORed)
     *      false, if none of the route entries that use nextHop as their next hop
     *            is marked as active
     *
     */
    bool getActiveStateForNextHop(const IPv6Address& nextHop);

    void setActiveStateForNextHop(const IPv6Address& nextHop, bool value);

    /**
     * Creates a static route. If metric is omitted, it gets initialized
     * to the interface's metric value.
     */
    virtual route_key_t addRoute(const IPv6Address* destPrefix, uint8_t prefixLength,
                        const IPv6Address* nextHop, uint16_t interfaceId,
                        int metric = 0);

    virtual void clearRouteList();

//    virtual IPv6Address & getNextHop(const IPv6Address &address);

    //FABIAN
    virtual route_key_t modifyRoute(const IPv6Address* destPrefix, uint8_t prefixLength,
            const IPv6Address* nextHop, uint16_t interfaceId,
            int metric = 0);

    bool deleteRoute(IPv6Address ip);//, DAO_Object DAO_info);

    virtual void updateDefaultRoute(const IPv6Address * destPrefix,
            uint16_t interfaceId,
            int metric);

    void deleteDefaultRoute();

    const IPv6Route* getDefaultRoute() const { return defaultRoute;}

protected:
    virtual void initialize();
    route_key_t internAddRoute(IPv6Route * route);

    RouteList routeList;
    IPv6InterfaceTable* it;

    //FABIAN
    IPv6Route *defaultRoute;

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
