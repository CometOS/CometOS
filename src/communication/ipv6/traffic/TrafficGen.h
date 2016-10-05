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

#ifndef TRAFFIC_GEN_H_
#define TRAFFIC_GEN_H_

#include "cometos.h"
#include "UDPLayer.h"
#include "ICMPv6.h"
#include "LongScheduleModule.h"
#include "Statistics.h"
#include "LowpanAdaptionLayer.h"
#include "MacStats.h"
#include "SList.h"
#include "TrafficGenListener.h"

#define NUM_NODES 7                     ///< Max 254
#define MT_TEST_MESSAGE_INFO_SIZE 8
#define MT_TEST_MESSAGE_MAX_PARTS 8

#ifndef BASESTATION_ADDR
#define BASESTATION_ADDR 0
#endif



struct TgResults {
    TgResults() {}

    cometos_v6::UdpLayerStats udpStats;
//    cometos_v6::LowpanAdaptionLayerStats lowpanStats;
    cometos::MacStats macStats;
};

struct StackCfg {
    StackCfg()
    {}

    cometos_v6::UdpConfig udpCfg;
    cometos_v6::IpConfig ipCfg;
};



enum {
    IM_FIXED = 0,
    IM_UNIFORM = 1,
    IM_POISSON = 2
};
typedef uint8_t intervalMode_t;

struct TgConfig {
        TgConfig(timeOffset_t offset = 0,
        uint16_t payloadSize = 0,
        bool mRTT = 0,
        uint16_t rttDest = 0,
        bool mTP = 0,
        uint16_t tpDest = 0,
        timeOffset_t rate = 0,
        intervalMode_t mode = 0,
        uint16_t maxRuns = 0,
        timeOffset_t finishDelay = 0) :
            offset(offset),
            payloadSize(payloadSize),
            mRTT(mRTT),
            rttDest(rttDest),
            mTP(mTP),
            tpDest(tpDest),
            rate(rate),
            mode(mode),
            maxRuns(maxRuns),
            finishDelay(finishDelay)
    {}

    timeOffset_t offset;
    uint16_t payloadSize;
    bool mRTT;
    uint16_t rttDest;
    bool mTP;
    uint16_t tpDest;
    timeOffset_t rate;
    intervalMode_t mode;
    uint16_t maxRuns;
    timeOffset_t finishDelay;
};



class TrafficGen: public cometos::LongScheduleModule, public cometos_v6::ICMPv6Listener, public cometos_v6::UDPListener {
public:

    static const char *const MODULE_NAME;

    TrafficGen(const char * service_name = MODULE_NAME);
    virtual ~TrafficGen() {}

    virtual void initialize();

    void common_init();

    virtual void finish();

    void udpTimerFired(cometos::LongScheduleMsg * msg);
    void echoTimerFired(cometos::LongScheduleMsg * msg);

    cometos_v6::BufferInformation* getDataBuffer(uint16_t size);

    void sendMessage();

    void setSeqNumAndSendMessage(uint16_t seqNum);

    void measureRTT();

    TgResults & getAndResetResults();

    void udpPacketReceived(const IPv6Address& src,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length);

    virtual void handleIncomming(const IPv6Address& src,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length);



    void receiveEchoReply(uint16_t seqNum, bool success);

    virtual void icmpMessageReceived(const IPv6Address& src,
            const IPv6Address& dst,
            uint8_t type,
            uint8_t code,
            const uint16_t additional[2],
            const uint8_t* data,
            uint16_t length);

    void transferFinishedDispatch();

    virtual void transferFinished(node_t id, TgResults & results);

    virtual void finishTimerFired(cometos::Message * msg);

    bool run(TgConfig  & cfg);

    SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> getRttStats();

    bool reset();

    bool subscribeToIncommingDatagramEvent(TrafficGenListener * mtl);

    cometos_error_t setStackCfg(StackCfg& cfg);

    StackCfg getStackCfg();

    cometos_error_t resetStackCfg();

protected:

    TgResults results;

    time_ms_t getCommonTime();

    double doublerand();

    cometos::LongScheduleModule::longTimeOffset_t nextOffset();

    void setConfig(TgConfig & cfg);

    typedef void(TrafficGen::*pMtMethod)(cometos::LongScheduleMsg *);

    class StartExperimentMsg : public cometos::Message {
    public:
        StartExperimentMsg(pMtMethod m) :
            m(m)
        {}

        pMtMethod getMethodToExecute() {
            return m;
        }

    private:
        pMtMethod m;
    };

    virtual void offsetTimerFired(StartExperimentMsg * msg);

    bool startExperiment(TgConfig & cfg, pMtMethod m);

    cometos_v6::UDPLayer*   udp;
    cometos_v6::ICMPv6*     icmp;

    uint16_t port;

    bool        mRTT;
    uint16_t    rttDest;

    bool        mTP;
    uint16_t    tpDest;

    uint16_t    ratePer256s;

    intervalMode_t mode;

    static const uint16_t RATE_FACTOR = 256;

    uint16_t    payloadSize;

    uint16_t    maxRuns;

    uint16_t     tries;

    uint16_t    seqNum;

    uint8_t     splitDatagramInto;

    timeOffset_t finishDelay;

    uint16_t currPkt;

    bool isActive;

    bool isFirst;

    unsigned long   errors;
    unsigned long   unable;
    unsigned long   timedout;
    unsigned long   rtt;
    unsigned long   rttMax;
    unsigned long   rttMin;
    unsigned long   successful;

    cometos::LongScheduleMsg * udpTimer;

    SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> rttStats;

    cometos::RemoteEvent<TgResults> tfEvent;

private:
    cometos::StaticSList<TrafficGenListener *, 2> listeners;
};



namespace cometos {

void serialize(ByteVector& buffer, const TgConfig & value);
void unserialize(ByteVector& buffer, TgConfig & value);

void serialize(ByteVector& buffer, const StackCfg & value);
void unserialize(ByteVector& buffer, StackCfg & value);

void serialize(ByteVector& buffer, const TgResults & value);
void unserialize(ByteVector& buffer, TgResults & value);

void serialize(ByteVector & buf, const SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> & val);
void unserialize(ByteVector & buf, SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> & val);
}

#endif /* TRAFFIC_GEN_H_ */
