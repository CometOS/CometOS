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


#ifndef __COMETOS_V6_NEIGHBOR_DISCOVERY_H_
#define __COMETOS_V6_NEIGHBOR_DISCOVERY_H_

#include "cometos.h"
#include "IPv6Address.h"
#include "SList.h"
#include "MacAbstractionBase.h"
#include "Ieee802154MacAddress.h"

namespace cometos_v6 {

#define NEIGHBOR_DISCOVERY_MODULE_NAME "nd"

#define IP_NWK_PREFIX 0x64,0x29,0x30,0x31,0x0,0x0,0x0

const uint8_t ND_MAX_ENTRIES = 18;

struct NeighborEntry {
    Ieee802154MacAddress mac;
    IPv6Address ip;
    NeighborEntry()
    {}

    NeighborEntry(const IPv6Address& ip):
        mac(ip.getAddressPart(7)),
        ip(ip)
    {}

    NeighborEntry(const Ieee802154MacAddress& mac, const IPv6Address& ip):
        mac(mac),
        ip(ip)
    {}

    bool operator==(const IPv6Address& ip) const {
        return (ip == this->ip);
    }

    bool operator==(const Ieee802154MacAddress& mac) const {
        return (mac == this->mac);
    }

    void operator=(const NeighborEntry& other) {
        ip = other.ip;
        mac = other.mac;
    }
};

class NeighborDiscovery : public cometos::Module
{
  public:
    NeighborDiscovery(const char * service_name = NULL):
        cometos::Module(service_name),
        macBroadcast(MAC_BROADCAST, MAC_BROADCAST, MAC_BROADCAST, MAC_BROADCAST),
        autoInsert(true)
    {}

    ~NeighborDiscovery() {
    }

    typedef cometos::StaticSList<NeighborEntry, ND_MAX_ENTRIES> NeighborList;

    const Ieee802154MacAddress* findNeighbor(const IPv6Address & nextHop);

    const Ieee802154MacAddress* resolveNeighbor(const IPv6Address & nextHop) ;

    const IPv6Address* reverseLookup(const Ieee802154MacAddress& macAddr);

    bool addNeighbor(const IPv6Address& ip, const Ieee802154MacAddress& mac) {
        for (uint8_t i=neighbors.begin(); i != neighbors.end(); i = neighbors.next(i))
        {
            if (neighbors[i] == ip) {
                neighbors[i].mac = mac;
                return true;
            }
        }
        return (neighbors.push_front(NeighborEntry(mac, ip)) != neighbors.end());
    }

    void removeNeighbor(const Ieee802154MacAddress& mac) {
        uint8_t i=neighbors.begin();
        while (i != neighbors.end())
        {
            if (neighbors[i].mac == mac) {
                i = neighbors.erase(i);
            } else {
                i = neighbors.next(i);
            }
        }
    }

  protected:
    virtual void initialize();

    NeighborList neighbors;
    Ieee802154MacAddress macBroadcast;

    bool autoInsert;
};


}
#endif
