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

#ifndef AODV_H_
#define AODV_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "Layer.h"
#include "NwkHeader.h"
#include "BitAgingSList.h"
#include "Tuple.h"
#include "SList.h"


/*MACROS---------------------------------------------------------------------*/

#define ROUTING_HISTORY_SIZE		15
#define ROUTING_HISTORY_REFRESH		5000
#define ROUTING_TABLE_SIZE			15
#define ROUTING_TABLE_REFRESH		30000
#define ROUTING_MAX_COSTS			30


#define ROUTING_PACKET_QUEUE_SIZE	10
#define ROUTING_TIMEOUT				1000


namespace cometos {

/*TYPES----------------------------------------------------------------------*/
typedef Tuple<node_t, uint8_t> historyEntry_t;
typedef Tuple<node_t, node_t, uint8_t> tableEntry_t;

/*PROTOTYPES-----------------------------------------------------------------*/


class AODV: public Layer {
public:

	AODV(const char * name = NULL);

	~AODV();

	/**@inheritDoc
	 */
	virtual void initialize();


	/**@inheritDoc
	 */
	virtual void handleRequest(DataRequest* msg);

	virtual void forwardRequest(DataRequest* msg);


	virtual void handleTimeout(DataRequest* msg);

	/**@inheritDoc
	 */
	virtual void handleIndication(DataIndication* msg);

	virtual void handleRreqIndication(DataIndication* msg);

	virtual void handleRrepIndication(DataIndication* msg);

	virtual void handleRerrIndication(DataIndication* msg);

	void handleResponse(DataResponse* resp);

	bool checkHeader(NwkHeader &nwk);

	void finish();

	void updateRoutingTable(node_t nextHop, node_t dst, uint8_t hops);

	InputGate<DataIndication> rreqIndIn;
	OutputGate<DataRequest> rreqReqOut;

	InputGate<DataIndication> rrepIndIn;
	OutputGate<DataRequest> rrepReqOut;

	InputGate<DataIndication> rerrIndIn;
	OutputGate<DataRequest> rerrReqOut;


private:

	node_t getNextHop(node_t dst);

	void historyUpdate(Message* timer);
	BitAgingSList<historyEntry_t, ROUTING_HISTORY_SIZE> history;

	void tableUpdate(Message* timer);
	BitAgingSList<tableEntry_t, ROUTING_TABLE_SIZE> table;

	uint8_t seq;

	typedef StaticSList<DataRequest*, ROUTING_PACKET_QUEUE_SIZE> aodv_queue_t;
	aodv_queue_t queue;

	uint16_t sendingOffset;

#ifdef ROUTING_ENABLE_STATS
	uint16_t forwarded;
	uint16_t control;
	uint16_t queueOverflow;
#endif

};

} /* namespace cometos */


#endif /* AODV_H_ */
