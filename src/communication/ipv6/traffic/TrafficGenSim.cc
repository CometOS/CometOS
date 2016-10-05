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

#include "TrafficGenSim.h"
#include "palId.h"
#include "XMLParseUtil.h"

using namespace omnetpp;

Define_Module(TrafficGenSim);

TrafficGenSim::~TrafficGenSim() {
    // TODO Auto-generated destructor stub
}

void TrafficGenSim::initialize() {
    cometos::LongScheduleModule::initialize();
    CONFIG_NED(startWith);
    CONFIG_NED(endWith);

    cXMLElement* routingFile;
    CONFIG_NED(routingFile);
    cXMLElementList xmlnodes = routingFile->getElementsByTagName(MT_NODE_ID_NAME);

    for (cXMLElementList::iterator itNodes = xmlnodes.begin(); itNodes != xmlnodes.end(); itNodes++) {
       node_t currNode = XMLParseUtil::getAddressFromAttribute(*itNodes, "id");

       recStats dummy;
       rcvdStats.set(currNode, dummy);

       char name[30];
       sprintf(name, "Lat_%d", currNode);
       cOutVector* vec = new cOutVector(name);
       if (latVectors.contains(currNode)) {
           delete vec;
       } else {
           latVectors.set(currNode, vec);
       }
    }

    common_init();
}

double TrafficGenSim::getVarFromSqrSum(double n, double sum, double sqrSum) {
    double var;
    if (n >= 2) {
         var = 1.0 / (n - 1.0) * (sqrSum - (1.0/n * sum * sum));
    } else {
        var = 0;
    }
    return var;
}

void TrafficGenSim::finish() {
	if (udpTimer != NULL) {
	    LOG_DEBUG("Cancel udpTimer");
	    longCancel(udpTimer);
	    delete udpTimer;
	}
    recordScalar("Id", palId_id());

    if (mRTT) {
        recordScalar("RTT_avg", rtt);
        recordScalar("RTT_min", rttMin);
        recordScalar("RTT_max", rttMax);
        recordScalar("Err", errors);
        recordScalar("Unable", unable);
        recordScalar("TO", timedout);
        recordScalar("Succ", successful);
    }

    if (received) {

        cometos::StaticSList<node_t, NUM_MAX_NODES> nodes;
        rcvdStats.keysToList(nodes);

        // iterate through stats for nodes
        for (uint8_t i = nodes.begin(); i != nodes.end(); i = nodes.next(i)) {
            char name[30];
            node_t currNode = nodes.get(i);
            flushDatagramData(currNode);
            recStats* currNodeStats = rcvdStats.get(currNode);

            if (currNodeStats->latStats.n() > 0) {
                LOG_DEBUG("stats for node 0x" << std::hex << currNode << ": " << std::dec << currNodeStats->latStats.n());

                double min = currNodeStats->latStats.getMin();
                double max = currNodeStats->latStats.getMax();

                double n = currNodeStats->latStats.n();
                double sum = currNodeStats->latStats.getSum();
                double sqrSum = currNodeStats->latStats.getSqrSum();
                double var = getVarFromSqrSum(n, sum, sqrSum);

                double avg = sum / n;


                sprintf(name, "Lat_avg_%d", currNode);
                recordScalar(name, avg);
                sprintf(name, "Lat_min_%d", currNode);
                recordScalar(name, min);
                sprintf(name, "Lat_max_%d", currNode);
                recordScalar(name, max);
                sprintf(name, "Lat_dev_%d", currNode);
                recordScalar(name, sqrt(var));
            } else {
                sprintf(name, "Lat_avg_%d", currNode);
                recordScalar(name, -1);
                sprintf(name, "Lat_min_%d", currNode);
                recordScalar(name, -1);
                sprintf(name, "Lat_max_%d", currNode);
                recordScalar(name, -1);
                sprintf(name, "Lat_dev_%d", currNode);
                recordScalar(name, -1);
            }
            sprintf(name, "Num_%d", currNode);
            recordScalar(name, currNodeStats->numRcvd);
            sprintf(name, "Rcvd_%d", currNode);
            recordScalar(name, currNodeStats->receivedBytes);
        }
    }

    cometos::StaticSList<node_t, NUM_MAX_NODES> vecList;
    latVectors.keysToList(vecList);
    for (uint8_t it = vecList.begin(); it != vecList.end(); it = vecList.next(it)){
        delete (*latVectors.get(vecList.get(it)));
    }
    latVectors.clear();
    vecList.clear();
}

void TrafficGenSim::icmpMessageReceived(const IPv6Address& src,
        const IPv6Address& dst,
        uint8_t type,
        uint8_t code,
        const uint16_t additional[2],
        const uint8_t* data,
        uint16_t length) {
    LOG_INFO("Rcvd an ICMP Dst Unrchbl Msg of Code "
            << code << " from " << src.getAddressPart(7));
    errors++;
}

void TrafficGenSim::handleIncomming(const IPv6Address& src,
        uint16_t srcPort,
        uint16_t dstPort,
        const uint8_t* data,
        uint16_t length)
{
    LOG_DEBUG("mt.handleIncomming from: " << src.str() << " length: " << length);
    LOG_DEBUG("ptr=" << std::hex << (uintptr_t) data);
    uint16_t from = src.getAddressPart(7);
    received = true;
    if (length >= MT_TEST_MESSAGE_INFO_SIZE) {
        uint16_t currSeq = ((uint16_t)data[0] << 8)|(data[1]);
        uint32_t srcTs = ((uint32_t) data[2] << 24) | ((uint32_t) data[3] << 16) | ((uint32_t) data[4] << 8) | data[5];
        uint8_t  fragNum = data[6];
        uint8_t  numFrags = data[7];
        LOG_DEBUG("currSeq=" << currSeq);

        if (currSeq >= startWith && currSeq < endWith) {
            recStats* rs = rcvdStats.get(from);

            ASSERT(rs != NULL);
            uint32_t localTs = palLocalTime_get();
            ASSERT(localTs >= srcTs);
            uint32_t latency = localTs - srcTs;

            LOG_DEBUG("srcTs=" << srcTs << "|localTs=" << localTs << "|lat=" << latency);

            LOG_DEBUG("ptr=" << std::hex << (uintptr_t) data);
            LOG_DEBUG("from=" << std::hex << from << " SeqNum=" << std::dec << currSeq);


            DatagramStats & dgs = rs->currDg;

            // new datagram, record stats for previous one and reset datagram stats
            if (currSeq > rs->currSeqNum) {
                flushDatagramData(from);
                rs->currSeqNum = currSeq;
            }

            // record statistics to temporary struct
            if (rs->currSeqNum == currSeq) {
                if (dgs.rcvdFrags.get(fragNum)) {
                    LOG_WARN("Rcvd dup");
                } else {
                    rs->receivedBytes += length;
                    LOG_DEBUG("Pckt Rcvd. Sum of Rcvd Bytes: " << rs->receivedBytes);

                    dgs.numFragsPerSeq = numFrags;
                    ASSERT(fragNum < numFrags);
                    ASSERT(fragNum < MT_TEST_MESSAGE_MAX_PARTS);
                    dgs.rcvdFrags.set(fragNum, true);
                    dgs.latencies.push_back(latency);
                }
            } else {
                LOG_WARN("Rcvd ooo");
            }
        }
    }
}

void TrafficGenSim::flushDatagramData(node_t from) {
    recStats * rs = rcvdStats.get(from);
    DatagramStats & dgs = rs->currDg;
    if (dgs.rcvdFrags.count(true) > 0) {
        uint32_t lastLat = 0;
        uint32_t actualLat = 0;

        rs->numRcvd += ((double) dgs.rcvdFrags.count(true)) / dgs.numFragsPerSeq;
        // process all latencies for this particular datagram and its fragments
        for (uint8_t k = dgs.latencies.begin(); k != dgs.latencies.end(); k = dgs.latencies.next(k)){
            uint32_t lat = dgs.latencies.get(k);
            actualLat += lat - lastLat;
            lastLat = lat;
        }

        // only record latencies for completely received datagrams to prevent biasing
        if (dgs.rcvdFrags.count(true) == dgs.numFragsPerSeq) {
            LOG_DEBUG("Record latency: " << actualLat);
            rs->latStats.add(actualLat);
            recordLatency(from, actualLat);
        }
    }

    // reset datagram stats
    dgs.reset();
}

bool TrafficGenSim::recordLatency(node_t srcId, uint16_t latency) {
    cOutVector* latencyVector = *latVectors.get(srcId);
    if (latencyVector != NULL) {
        return latencyVector->record((double)latency);
    } else {
        return false;
    }
}
