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

#ifndef SectorLocationBasedRouting_H_
#define SectorLocationBasedRouting_H_

#include "Layer.h"
#include "BitAgingSList.h"
#include "NwkHeader.h"

#include <map>
#include <vector>

#define ROUTING_HISTORY_SIZE    20
#define ROUTING_HISTORY_REFRESH  5000

// number of replies a node is allowed to send
#define MAX_REPLIES 2

/*TYPES----------------------------------------------------------------------*/
typedef Tuple<int64_t, int64_t> position_t;

typedef Tuple<node_t, uint8_t> historyEntry_t;

typedef Tuple<uint8_t, uint8_t> sectorSpan_t;

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

template<int SIZE>
class MemoryCorruptionTest {
public:
    MemoryCorruptionTest() {
        for (int i = 0; i < SIZE; i++) {
            array[i] = 0xAA + i % 8;
        }
    }

    void check() {
        for (int i = 0; i < SIZE; i++) {
            ASSERT(array[i]==0xAA+i%8);
        }
    }

private:
    uint8_t array[SIZE];
};

// TODO How to remove bad neighbors in terms of progress
class SectorLocationBasedRouting: public Layer {

private:

    position_t ownPosition;

    std::vector<node_t> nextHops;

    std::map<node_t, double> candidates;
    std::map<node_t, double> establishedMap;
    DataRequest* currentRequest;
    sectorSpan_t currentSectors;
    std::list<DataRequest*> queue;
    node_t currentCandidate;bool waitingForReply;
    uint16_t sequence;

    /**tries to process next packet in queue, if protocol is already processing
     * a packet, method returns without any change*/
    void startProcessing();

    /**forward packet*/
    void forward();

    /**finish forwarding procedure*/
    void finishProcessing(bool success);

    uint8_t getSector(node_t dst,bool inverse=false);

    double getDistance(node_t dst);

    void addCandidate(node_t dst, position_t& posDst, bool established);

    void sendToCandidate();

    BitAgingSList<historyEntry_t, ROUTING_HISTORY_SIZE> history;

public:

    SectorLocationBasedRouting();

    void initialize();

    void finish();

    void historyUpdate(Message* timer);

    /**@inherit Layer*/
    virtual void handleRequest(DataRequest* msg);

    /**@inherit Layer*/
    virtual void handleIndication(DataIndication* msg);

    void handleIndication(DataIndication* pkt, NwkHeader& nwk);

    void handleSectorRequest(DataIndication* pkt);

    void handleSectorReply(DataIndication* pkt);

    void handleRequest(DataRequest* msg, NwkHeader& nwk);

    void handleResponseCandidate(DataResponse *resp);
    void handleResponse(DataResponse *resp);
    void handleResponseDirect(DataResponse *resp);
    void handleResponseReply(DataResponse *resp);
    void handleResponseRequest(DataResponse *resp);


    void handleDiscoveryTimeout(Message* msg);

    void slotTimeout(Message *timer);

    MemoryCorruptionTest<50> memCheck1;
    InputGate<DataIndication> sectorRequestIn;

    MemoryCorruptionTest<50> memCheck2;
    OutputGate<DataRequest> sectorRequestOut;

    InputGate<DataIndication> sectorReplyIn;
    OutputGate<DataRequest> sectorReplyOut;

    node_t getNextHop(node_t dst);

    position_t sub(const position_t & a, const position_t & b) {
        position_t res;
        res.first = a.first - b.first;
        res.second = a.second - b.second;
        return res;
    }

    double dot(const position_t & a, const position_t & b) {
        return a.first * b.first + a.second * b.second;
    }

protected:

#ifdef ROUTING_ENABLE_STATS
    uint16_t control;
    uint16_t forwarded;
    uint16_t discoveries;
#endif

    bool forceTransmission;
    uint8_t alreadySendingReply;

    static std::map<int, position_t> locationTable;

};

} /* namespace cometos */

#endif /* SectorLocationBasedRouting_H_ */
