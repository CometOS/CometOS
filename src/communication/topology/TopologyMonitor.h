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

#ifndef TOPOLOGY_MONITOR_H_
#define TOPOLOGY_MONITOR_H_

#include "Endpoint.h"
#include "Serializable.h"
#include "Statistics.h"
#include "MacAbstractionBase.h"
#include "HashMap.h"
#include "palLed.h"

#define TM_LIST_SIZE 28

namespace cometos {

struct TmConfig {
    TmConfig(timeOffset_t beaconInterval = 0, pktSize_t beaconSize = 0) :
        beaconInterval(beaconInterval),
        beaconSize(beaconSize)
    {}

    timeOffset_t beaconInterval;
    pktSize_t beaconSize;
};

struct TMNodeId {
    TMNodeId(node_t id = MAC_BROADCAST) :
        id(id)
    {}

    ~TMNodeId() {

    }

    bool operator==(const TMNodeId& rhs) {
        return id == rhs.id;
    }

    node_t id;
};

node_t operator%(const TMNodeId& lhs, const TMNodeId& rhs);



template<uint16_t TOPOLOGY_MONITOR_TABLE_SIZE>
class TopologyMonitor: public Endpoint {

public:
    typedef SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t> TmNeighborInfo;

    TopologyMonitor(const char * name = NULL) :
        Endpoint(name),
        numSent(0)
    {}

    virtual void initialize() {
        Endpoint::initialize();
        remoteDeclare(&TopologyMonitor::getInfo, "gi");
        remoteDeclare(&TopologyMonitor::getNodeList, "gnl");
        remoteDeclare(&TopologyMonitor::getNumSent, "gns");
        remoteDeclare(&TopologyMonitor::start, "start");
        remoteDeclare(&TopologyMonitor::stop, "stop");
        remoteDeclare(&TopologyMonitor::clear, "clear");
    }

	virtual void handleIndication(DataIndication* msg) {
	    mac_dbm_t currRssi = 0;
	    bool hasRxInfo = msg->has<MacRxInfo>();
	    LOG_DEBUG("Received indication src=" << msg->src << "hasRxInfo=" << hasRxInfo);
        if (hasRxInfo) {
            MacRxInfo * info = msg->get<MacRxInfo>();
            currRssi = info->rssi;
        }

        TMNodeId id(msg->src);

	    if (data.contains(id)) {
	        LOG_DEBUG("Add rssi " << (int) currRssi << " to node with id " << id.id);
	        if (currRssi != RSSI_INVALID) {
	            data.get(id)->add(currRssi);
	        }
	    } else {
	        TmNeighborInfo tmp;
	        if (currRssi != RSSI_INVALID) {
	            tmp.add(currRssi);
	        }
	        data.set(id, tmp);
	    }

	    delete(msg);
	}


	uint32_t getNumSent() {
	    return numSent;
	}


	TmNeighborInfo getInfo(TMNodeId & id) {
	    TmNeighborInfo info;
	    if (data.contains(id)) {
	        return *(data.get(id));
	    } else {
	        return info;
	    }
	}

	StaticSList<TMNodeId, TOPOLOGY_MONITOR_TABLE_SIZE> getNodeList() {
	    StaticSList<TMNodeId, TOPOLOGY_MONITOR_TABLE_SIZE> list;
	    data.keysToList(list);
	    for (uint8_t it = list.begin(); it != list.end(); it=list.next(it)) {
	        LOG_DEBUG("List Entry for neighbor: " << list.get(it).id);
	    }
	    return list;
	}

	bool start(TmConfig & cfg) {
	    if (cfg.beaconSize > AIRFRAME_MAX_SIZE) {
			return false;
		} 
		
		this->cfg.beaconInterval = cfg.beaconInterval;
		this->cfg.beaconSize = cfg.beaconSize;
		
	    if (!isScheduled(&beaconTimer)) {
	        schedule(&beaconTimer, &TopologyMonitor::timerFired);
	    }
	    return true;
	}

	bool stop() {
	    if (isScheduled(&beaconTimer)) {
	        cancel(&beaconTimer);
	    }
	    return true;
	}

	bool clear() {
	    StaticSList<TMNodeId, TOPOLOGY_MONITOR_TABLE_SIZE> list;
	    data.keysToList(list);
	    for (uint8_t it = list.begin(); it != list.end(); it = list.next(it)) {
	        data.get(list.get(it))->reset();
	    }
	    data.clear();
	    numSent = 0;
	    return true;
	}

	void timerFired(Message * msg) {
	    schedule(&beaconTimer, &TopologyMonitor::timerFired, cfg.beaconInterval);
	    numSent++;
	    cometos::AirframePtr frame = make_checked<Airframe>();
	    ASSERT(cfg.beaconSize <= AIRFRAME_MAX_SIZE);
		for (pktSize_t i = 0; i < cfg.beaconSize; i++) {
			*frame << i;
		}
	    sendRequest(new DataRequest(MAC_BROADCAST, frame), intrand(cfg.beaconInterval));
	}


private:

	HashMap<TMNodeId, TmNeighborInfo, TOPOLOGY_MONITOR_TABLE_SIZE> data;
	Message beaconTimer;
	TmConfig cfg;
	uint32_t numSent;
};

void serialize(ByteVector & buf, const StaticSList<TMNodeId, TM_LIST_SIZE> & list);
void unserialize(ByteVector & buf, StaticSList<TMNodeId, TM_LIST_SIZE> & list);

void serialize(ByteVector & buf, const TmConfig & val);
void unserialize(ByteVector & buf, TmConfig & val);

void serialize(ByteVector & buf, const TMNodeId & val);
void unserialize(ByteVector & buf, TMNodeId & val);

void serialize(ByteVector & buf, const SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t> & val);
void unserialize(ByteVector & buf, SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t> & val);



} /* namespace cometos */
#endif /* TOPOLOGY_MONITOR_H_ */
