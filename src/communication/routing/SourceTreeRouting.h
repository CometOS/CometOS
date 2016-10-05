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

#ifndef STATICSOURCETREEROUTING_H_
#define STATICSOURCETREEROUTING_H_

#include "Layer.h"
#include "Tuple.h"
#include "BitAgingSList.h"
#include "NwkHeader.h"
#include "Vector.h"

#include <map>

// maximal length of source route
#define SOURCE_ROUTING_LENGTH	10

#define ROUTING_HISTORY_SIZE		15
#define ROUTING_HISTORY_REFRESH		5000


/*TYPES----------------------------------------------------------------------*/
typedef Tuple<node_t, uint8_t> historyEntry_t;

/*PROTOTYPES-----------------------------------------------------------------*/

typedef cometos::Vector<node_t,SOURCE_ROUTING_LENGTH> path_t;

class PathObj: public cometos::Object {
public:
	virtual cometos::Object* getCopy() const {
		PathObj* obj= new PathObj;
		obj->path=this->path;

		return obj;
	}
	path_t path;
};


/**Source routing module which must be connected to a
 * tree routing algorithm.
 *
 * Note that MetaData is appended to the Airframe,
 * thus the Tree algorithm is not allowed to remove
 * these data.
 *
 * Usage:
 *
 * ----------------------
 * |    Application     |
 * ----------------------
 *         |
 *  ---------------------
 *  | SourceTreeRouting |
 *  ------------------- |
 *         |          | |
 *  ---------------   | |
 *  |   Tree      |   | |
 *  ---------------   | |
 *         |          | |
 *  ------------------- |
 *  |                   |
 *  ---------------------
 *       |   |
 * ----------------------
 * |       MAC          |
 * ----------------------
 */
class SourceTreeRouting: public cometos::Module {
public:

	SourceTreeRouting(const char *name = NULL);

	~SourceTreeRouting() {
	}

	virtual void initialize();

	virtual void finish();

	virtual void handleRequest(cometos::DataRequest* msg);

	virtual void handleSourceIndication(cometos::DataIndication* msg);

	virtual void handleTreeIndication(cometos::DataIndication* msg);

	// gates to upper layer
	cometos::InputGate<cometos::DataRequest> gateReqIn;
	cometos::OutputGate<cometos::DataIndication> gateIndOut;

	// Gates to lower layer
	cometos::InputGate<cometos::DataIndication> sourceIndIn;
	cometos::OutputGate<cometos::DataRequest> sourceReqOut;

	cometos::InputGate<cometos::DataIndication> treeIndIn;
	cometos::OutputGate<cometos::DataRequest> treeReqOut;

	// inner tree routing algorithm
	void handleInnerRequest(cometos::DataRequest* msg);
	void handleInnerIndication(cometos::DataIndication* msg);

	cometos::InputGate<cometos::DataIndication>  innerIndIn;
	cometos::OutputGate<cometos::DataRequest>    innerReqOut;
	cometos::InputGate<cometos::DataRequest>  innerReqIn;
	cometos::OutputGate<cometos::DataIndication> innerIndOut;


private:

	bool isSink;

	void historyUpdate(cometos::Message* timer);
	cometos::BitAgingSList<historyEntry_t, ROUTING_HISTORY_SIZE> history;

	uint8_t seq;

#ifdef ROUTING_ENABLE_STATS
    uint16_t forwarded;
#endif

	// used by sink
	std::map<node_t, path_t> paths;
};

#endif /* STATICSOURCETREEROUTING_H_ */
