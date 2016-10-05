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
 * @author Stefan Unterschuetz
 */

#ifndef SIMPLEROUTING_H_
#define SIMPLEROUTING_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "Layer.h"
#include "NwkHeader.h"
#include "BitAgingSList.h"
#include "Tuple.h"


/*MACROS---------------------------------------------------------------------*/

#define SIMPLE_ROUTING_HISTORY_SIZE		25
#define SIMPLE_ROUTING_TABLE_SIZE		25



namespace cometos {

/*TYPES----------------------------------------------------------------------*/
typedef Tuple<node_t, uint8_t> historyEntry_t;
typedef Tuple<node_t, node_t, uint8_t> tableEntry_t;

/*PROTOTYPES-----------------------------------------------------------------*/

/**CometOS SimpleRouting, an easy routing protocol for small multi-hop networks
 *
 * Properties:
 * <li> duplicate filter with aging functionality
 * <li> routing table with aging functionality
 *
 * <li> no control messages
 * <li> no reliability
 * <li> no prerequisites
 *
 * CometOS SimpleRouting is an enhancement of flooding. By default, packets are
 * transmitted via flooding. During this flooding procedure routing tables are
 * filled. If a node finds an entry in its routing table, it will use unicast.
 * In case of transmission errors, a node switches back to flooding.
 *
 *
 * Protocol Operations:
 *
 * Network Header: | src |	dst |	seq | hops (1+7 bit) |
 *
 * All received packets additionally contain one hop address of sender and intended receiver.
 * Furthermore, a duplicate filter is supported on the network layer. The filter
 * uses the dst and seq field. A simple aging algorithm is used to discard old entries
 * (see bit-aging).
 *
 * Flooding: If destination address contains broadcast address, flooding is used. Flooding is
 * always executed with a random offset of (0, T_f). Cost field is initialized with
 * zero. Nodes receiving a packet for the first time, try to estimate the link quality to the
 * sender. If this is insufficient the packet is discarded. Otherwise the routing table is
 * updated by adding the tuple (src, sender). Similar to the duplicate filter the bit-aging
 * algorithm can be applied (e.g. with T_a=60 s). If routing table is full, a random entry is
 * removed. After updating the routing table the packet is forwarded. Packets that are already
 * processed (entry in duplicate filter) are immediately discarded. No forwarding is done if
 * either destination is reached (except of broadcasts) or the costs of the packets are higher
 * then COST_MAX. The programmer should take care that no overflow of cost field happens,
 * furthermore the most most significant bit must be zero in this routing mode.
 *
 * Routing: Used by source node is destination is not a broadcast address and destination is
 * in routing table. Packet is sent via unicast (with acknowledgment). On the route the age
 * fields of the forwarders are updated (set to zero).
 *
 * Fallback Flooding: If routing is used, but next hop is not available (error from MAC),
 * then flooding is used with a starting cost value of zero but with the most significant bit
 * set to one. Node receiving this packet will remove the corresponding entry of the routing
 * table. This done due to the fact that current routing information may be invalid. However,
 * the max forwarding boundary of COST_MAX is still applied
 *
 * bit-aging: Each entry contains one bit used as aging flag. For new or updated entries this
 * flag is set to zero. A periodic timer of interval T_a is run. On timeout, all flags containing
 * a zero are set to one. Entries containing a flag with a value of one are removed. Maximal
 * lifetime of an entry is (T_a,2T_a). T_a can be a high value, e.g. 10 seconds. Advantages
 * of bit-aging is the low computation overhead and memory demand.
 *
 */
class SimpleRouting: public Layer {
public:

	SimpleRouting(const char * name = NULL);

	~SimpleRouting();

	/**@inheritDoc
	 */
	virtual void initialize();


	/**@inheritDoc
	 */
	virtual void handleRequest(DataRequest* msg);

	/**@inheritDoc
	 */
	virtual void handleIndication(DataIndication* msg);

	void handleResponse(DataResponse* resp);


	void sendAutoReply(node_t dst);
	

	void finish();
private:

	node_t getNextHop(node_t dst);

	void historyUpdate(Message* timer);
	BitAgingSList<historyEntry_t, SIMPLE_ROUTING_HISTORY_SIZE> history;

	void tableUpdate(Message* timer);
	BitAgingSList<tableEntry_t, SIMPLE_ROUTING_TABLE_SIZE> table;

	uint8_t seq;

#ifdef ROUTING_ENABLE_STATS
	uint16_t forwarded;
#endif

};

} /* namespace cometos */

bool operator==(const cometos::tableEntry_t&val1, const node_t& val2);

#endif /* SIMPLEROUTING_H_ */
