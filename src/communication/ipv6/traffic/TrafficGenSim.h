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
 * @author Martin Ringwelski, Andreas Weigel
 */

#ifndef TRAFFIC_GEN_SIM_H_
#define TRAFFIC_GEN_SIM_H_

#include "TrafficGen.h"
#include "HashMap.h"
#include "Statistics.h"

#define NUM_MAX_NODES 100

#define MT_NODE_ID_NAME "node"

class TrafficGenSim: public TrafficGen {
    struct DatagramStats {
       DatagramStats() :
           numFragsPerSeq(1)
       {}

       void reset() {
           numFragsPerSeq = 1;
           latencies.clear();
           rcvdFrags.fill(false);
       }

       cometos::BitVector<MT_TEST_MESSAGE_MAX_PARTS> rcvdFrags;
       uint16_t numFragsPerSeq;
       cometos::StaticSList<uint32_t, MT_TEST_MESSAGE_MAX_PARTS> latencies;
    };

    struct recStats {
        unsigned long   receivedBytes;
        int32_t currSeqNum;
        double numRcvd;
        DatagramStats currDg;
        SumsMinMax<uint32_t, uint16_t, uint64_t, uint64_t> latStats;

        recStats ():
            receivedBytes(0),
            currSeqNum(-1),
            numRcvd(0)
        {}

        recStats& operator=(recStats& other) {
            receivedBytes = other.receivedBytes;

            latStats = other.latStats;
            return *this;
        }
    };
public:
    TrafficGenSim(const char * service_name = NULL):
        TrafficGen(service_name), received(false), startWith(0), endWith(5000)
    {}
    virtual ~TrafficGenSim();
    virtual void initialize() override;
    virtual void finish() override;

    virtual void handleIncomming(const IPv6Address& src,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length) override;

    virtual void icmpMessageReceived(const IPv6Address& src,
            const IPv6Address& dst,
            uint8_t type,
            uint8_t code,
            const uint16_t additional[2],
            const uint8_t* data,
            uint16_t length) override;

    virtual void flushDatagramData(node_t from);

    virtual void finishTimerFired(cometos::Message * msg) override {
        delete(msg);
    }

    static double getVarFromSqrSum(double n, double sum, double sqrSum);

protected:
    bool received;


    uint16_t startWith;
    uint16_t endWith;

    cometos::HashMap<node_t, recStats, NUM_MAX_NODES>  rcvdStats;
#ifdef OMNETPP
    cometos::HashMap<node_t, omnetpp::cOutVector*, NUM_MAX_NODES> latVectors;
#endif

private:
    bool recordLatency(node_t srcId, uint16_t latency);

};

#endif /* TRAFFIC_GEN_SIM_H_ */
