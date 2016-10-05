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

#ifndef __COMETOS_V6_ROUTINGBASE_H_
#define __COMETOS_V6_ROUTINGBASE_H_

#include "cometos.h"
#include "IPv6Datagram.h"
#include "IPv6Address.h"
#include "LinkLayerInformation.h"

#define ROUTING_MODULE_NAME   "rm"

namespace cometos_v6 {


/**
 * Base class for IPv6 routing protocol implementations. Offers an interface
 * for the forwarding engine to pass packets which are to be routed to the
 * routing protocol for inspection. This can be used for datapath validation
 * and link estimation.
 */
class RoutingBase {
public:
    virtual ~RoutingBase() {}

    /**
     * Signaled (called) when initiating a new flow (datagram coming "from above")
     *
     * Allows routing layer to modify payload or add additional headers
     * before the packet is sent.
     *
     * @param dg      datagram to be send
     * @param nextHop IPv6 address of the next hop TODO constant correct here?
     * @return true, if routing protocol approves sending,
     *         false if not (datagram should then be dropped)
     */
    virtual bool initiate(IPv6Datagram & dg, const IPv6Address & nextHop){
        return true;
    }


    /**
     * Signaled (called), when forwarding a datagram.
     *
     * Allows the routing protocol to inspect the datagram header as
     * it flows through. If the routing protocol returns false,
     * the packet should be dropped. The routing protocol may change
     * fields in the packet header and possibly the hop-by-hop-options.
     *
     * @param dg      datagram being forwarded
     * @param nextHop IPv6 address of the next hop
     */
    virtual bool approve(IPv6Datagram & dg, const IPv6Address & nextHop){
        return true;
    }


    /**
     * Signaled (called), once per forwarded packet by
     * the forwarding engine.
     *
     * Contains a data structure containing information about the link layer
     * efforts to transport the datagram, i.e., number of fragments created,
     * number of retransmissions, if transmission has been successful.
     *
     * @param dst   IPv6 address of the destination
     * @param info  information about the sending process at link-layer level
     */
    virtual void txResult(const IPv6Address & dst, const LlTxInfo & info) = 0;

    /**
     *
     */
    virtual void rxResult(const IPv6Address & src, const LlRxInfo & info) = 0;
};

}

#endif
