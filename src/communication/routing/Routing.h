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

#ifndef ROUTING_H_
#define ROUTING_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "Layer.h"
#include "DuplicateFilter.h"
#include "Queue.h"
#include "types.h"
#include "cometos.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "NwkHeader.h"
#include "RoutingInfo.h"


/*TYPES----------------------------------------------------------------------*/

struct packet_id_t {
	packet_id_t(uint16_t src = BROADCAST, uint8_t seq = 0) :
			src(src), seq(seq) {
	}
	uint16_t src;
	uint8_t seq;
};


/*MACROS---------------------------------------------------------------------*/

/**Size of the packet history of the routing layer. Not more than ROUTING_HISTORY_SIZE are allowed to
 * be "on the air" at the same time, otherwise errors might occur.
 */
#define ROUTING_HISTORY_SIZE 	20
#define ROUTING_HISTORY_REFRESH	 5000
#define ROUTING_MAX_HOPS		6

#define ROUTING_SENDING_OFFSET		100

/*PROTOTYPES-----------------------------------------------------------------*/

/**
 * Base class for all routing protocols. This implementation uses pure
 * flooding. Duplicate filtering is already included.
 *
 * This class follows a kind of  strategy pattern meaning a derived class is
 * able to implement specific parts of the routing process.
 */
class Routing: public cometos::Layer {
public:

	Routing(const char* name = NULL);

	/**@inheritDoc
	 */
	virtual void initialize();

	virtual void finish();

	/**Timeout is used to perform aging algorithm for Duplicate filter.
	 */
	void refresh(cometos::Message *timer);

	/**Handler for data from upper layer. In general this passes
	 * meesage to method.
	 * handleRequest(DataRequest* msg, NwkHeader& nwk)
	 */
	virtual void handleRequest(cometos::DataRequest* msg);

	/**
	 * Performs routing decision to send packet to given destination. In general
	 * it is only necessary for derived classes to override this method.
	 */
	virtual void handleRequest(cometos::DataRequest* msg, cometos::NwkHeader& nwk);

	/**Callback when receiving data from lower layer. This method will
	 * perform some preprocessing before invoking
	 * handleIndication(DataIndication* pkt, NwkHeader& nwk).
	 */
	virtual void handleIndication(cometos::DataIndication* msg);

	/**
	 * Main processing of received packets. This method will also perform
	 * routing decision. If message has to be further transmitted then
	 * handleRequest(DataRequest* msg, NwkHeader& nwk) is invoked
	 */
	virtual void handleIndication(cometos::DataIndication* pkt, cometos::NwkHeader& nwk);

	/**
	 * Checks whether packet is duplicate
	 */
	virtual bool isDuplicate(cometos::NwkHeader& nwk);

	/**
	 * Sending offset for BROADCAST messages.
	 */
	uint16_t sendingOffset;
	uint8_t maxHops;
	uint8_t sequence;

	uint16_t forwarded;

protected:
	timeOffset_t getFloodingOffset(cometos::NwkHeader & nwk);

	cometos::DuplicateFilter<packet_id_t, ROUTING_HISTORY_SIZE> history;

};

//bool operator==(const packet_id_t &op1, const packet_id_t & op2);

#endif /* ROUTING_H_ */
