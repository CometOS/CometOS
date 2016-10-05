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

#ifndef __COMETOS_V6_STATICROUTING_H_
#define __COMETOS_V6_STATICROUTING_H_

#include "RoutingBase.h"
#include "RoutingTable.h"
#include "RemoteModule.h"
#include "cometos.h"

struct RouteInfo {
    IPv6Address prefix;
    IPv6Address nextHop;
    uint8_t prefixLength;
    RouteInfo() :
        prefixLength(0)
    {}
    RouteInfo(const IPv6Address& pf, uint8_t length, const IPv6Address& nH) :
        prefix(pf),
        nextHop(nH),
        prefixLength(length) {}
};


namespace cometos_v6 {

/**
 * "Simple" static routing class which creates routing table
 * entries at initialization time and adds them to the routing
 * table. It will not change routes at any time during
 * execution.
 *
 * Only meant for the simulator -- hardware nodes need some other means
 * to establish static routes.
 */
class StaticRouting : public cometos::RemoteModule, public RoutingBase {

public:
    StaticRouting(const char * service_name = NULL, RoutingTable *rt = NULL):
        RemoteModule(service_name),
        rt(rt) {}

    virtual void initialize();

#ifdef PAL_PERS
    bool storeRoute(RouteInfo & route, uint8_t & pos);
    uint8_t getNumStored();
    RouteInfo getRoute(uint8_t & pos);
    bool clearStore();
#endif

    void loadRoutes(cometos::Message * msg);

    void clearRouting();
    void addRoute(RouteInfo & route);

    void txResult(const IPv6Address & dst, const LlTxInfo & info);

    void rxResult(const IPv6Address & src, const LlRxInfo & info);

private:
#ifndef OMNETPP
    void storeAddr(IPv6Address const & ipa, uint16_t persAddr);
    void loadAddr(IPv6Address & ipa, uint16_t persAddr);
#endif
    RoutingTable *rt;
};

}

namespace cometos {
void serialize(ByteVector& buffer, const RouteInfo & value);
void unserialize(ByteVector& buffer, RouteInfo & value);
}
#endif
