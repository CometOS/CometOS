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
 * @author: Martin Ringwelski
 */

#include "NeighborDiscovery.h"

namespace cometos_v6 {

Define_Module(NeighborDiscovery);

void NeighborDiscovery::initialize()
{
    Module::initialize();
    CONFIG_NED(autoInsert);
}

const Ieee802154MacAddress* NeighborDiscovery::findNeighbor(const IPv6Address& address) {
    bool originalAi = autoInsert;
    autoInsert = false;
    const Ieee802154MacAddress* n = resolveNeighbor(address);
    autoInsert = originalAi;
    return n;
}

const Ieee802154MacAddress* NeighborDiscovery::resolveNeighbor(const IPv6Address& address) {

    if (address.isMulticast() || address.isLocalBroadcast() ||
            address.isLocalRouterBroadcast() || address.isSiteRouterBroadcast()) {
        return &macBroadcast;
    }

    // for now, use last 16-bit part of IPAddress as MAC address
    LOG_DEBUG("Get MAC Addr for IP Addr");
    for (uint8_t i=neighbors.begin(); i != neighbors.end(); i = neighbors.next(i))
    {
        if (neighbors[i] == address) {
            return &(neighbors[i].mac);
        }
    }

    if (!neighbors.full() && autoInsert) {
        uint8_t k = neighbors.push_front(NeighborEntry(address));
        if (k != neighbors.end()) {
            LOG_INFO("Added " << cometos::hex << address.getAddressPart(7) << cometos::dec << " to nb");
            return &(neighbors[k].mac);
        }
    }
    LOG_WARN("No neighbor found");
    return NULL;
}


const IPv6Address* NeighborDiscovery::reverseLookup(const Ieee802154MacAddress& macAddr) {
    for (uint8_t i = neighbors.begin(); i != neighbors.end(); i = neighbors.next(i)) {
        if (neighbors[i] == macAddr) {
            return &(neighbors[i].ip);
        }
    }

    if (!neighbors.full() && autoInsert) {
        uint8_t k = neighbors.push_front(NeighborEntry(IPv6Address(IP_NWK_PREFIX, macAddr.a4())));
        ASSERT(k != neighbors.end());
        LOG_INFO("Added " << neighbors[k].ip.str() << " to neighbor discovery");
        return &(neighbors[k].ip);
    }
    LOG_WARN("Reverse lookup failed");
    return NULL;
}

}
