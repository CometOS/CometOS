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
 * @author Stefan Unterschütz
 */

#ifndef DHROUTING_H_
#define DHROUTING_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "Layer.h"
#include "SList.h"
#include <map>
#include <list>

namespace cometos {

/*MACROS---------------------------------------------------------------------*/
#define DHROUING_MAX_NBS	10
#define DHROUTING_MAX_TTL	2
#define DHROUTING_BASE_TIME		100
#define DHROUTING_MAX_HOPS      8

/*PROTOTYPES-----------------------------------------------------------------*/

class DHRoutingNeighbor {
public:
	node_t id;
	node_t next;
	uint8_t hops;
	uint8_t ttl;

	bool operator==(const node_t & node) {
		return id == node;
	}
};

/**DHRouting runs multiple hierarchy levels. The implementation of each layer
 * is equal.
 */
class DHRoutingLayer {
public:
	DHRoutingLayer(uint8_t nbRadius, uint8_t misRadius);

	~DHRoutingLayer();

	node_t getLowestNbId();

	DHRoutingLayer *next;

	void printNbs();

	void updateNbs();

	/**@returns next control packet*/
	DataRequest* run(bool permitted, node_t ownId, uint8_t level);

	void receive(DataIndication *ind, node_t ownId);

	node_t getGateway(node_t id);

	void deleteNb(node_t id);

	bool hasNb(node_t id);

	node_t getGateway();



	void updateNb(node_t id, node_t next, uint8_t hops);
	StaticSList<DHRoutingNeighbor, DHROUING_MAX_NBS> nbs;
private:

	uint8_t nbRadius;
	uint8_t misRadius;

	node_t getNeighbor(uint8_t index);
	uint8_t round;

};

/** Dynamic Hierarchical Routing. Manges multiple routing layers. On each
 *  layer a maximal independent set clustering is done. Cluster-heads of layer i
 *  are starting as ordinary nodes on layer i+1. The upper most layer contains only
 *  one cluster-head.
 */
class DHRouting: public Layer {
public:

	DHRouting();
	virtual ~DHRouting();

	void initialize();

	void finish();

	void handleIndication(DataIndication* msg);

	void handleRequest(DataRequest* msg);
    void handleRequest(DataRequest* msg, node_t source);

	void timeout(Message *timer);

	DHRoutingLayer* getLayer(uint8_t level);

	void receiveRouteUpdate(DataIndication *ind);
	//void receiveData(DataIndication *ind);

	DataRequest * addRoutingInformation(DataRequest *req, uint8_t level, uint8_t level_offset);

	DataRequest * addRoutingInformation(DataRequest *req,bool force=false);

	/**returns highest dominator of node of lowest id*/
	node_t getDominator(uint8_t &level);


    void handleDiscoveryIndication(DataIndication* msg);

    void handleUpdateIndication(DataIndication* msg);


    InputGate<DataIndication> discoveryIndIn;
    OutputGate<DataRequest> discoveryReqOut;

    InputGate<DataIndication> nbUpdateIndIn;
    OutputGate<DataRequest> nbUpdateReqOut;


private:

	enum {
		MSG_DISCOVERY = 0xFF
	};

	// subtypes of data messages
//	enum {
//		MSG_ROUTE_UPDATE, MSG_CLUSTER_UPDATE, MSG_DATA
//	};

	// paths to each node are only stored by sink
	std::map<node_t, std::list<node_t> > paths;

	bool nbDiscover;
	DHRoutingLayer* begin;

	uint8_t getNextExecLayer();
	uint16_t counter;

	double maxDiscoveryTime;

#ifdef ROUTING_ENABLE_STATS
    uint16_t forwarded;
    uint16_t control;
    uint16_t numControlRecv;
#endif
};


} // namespace
#endif /* DHROUTING_H_ */
