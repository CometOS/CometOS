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

#ifdef OMNETPP
#include <ctime>
#include <math.h>
#endif
#ifndef OMNETPP
#include "palLed.h"
#include "math.h"
#endif

#include "TrafficGen.h"
#include "palLocalTime.h"
#include "palId.h"
#include "LowpanBuffer.h"
#include "LowpanAdaptionLayer.h"
#include "IpForward.h"
#include "NeighborDiscovery.h"
#include "NetworkTime.h"
#include "CsmaMac.h"

#define MT_PORT ((uint16_t)0xF0B0)

Define_Module(TrafficGen);


const char * const TrafficGen::MODULE_NAME = "tg";

uint16_t dstAddress[8] = {
        IP_NWK_PREFIX
};



TrafficGen::TrafficGen(const char * service_name) :
            cometos::LongScheduleModule(service_name),
            udp(NULL),
            icmp(NULL),
            port(0),
            mRTT(false),
            rttDest(0),
            mTP(false),
            tpDest(0),
            ratePer256s(10),
            mode(IM_UNIFORM),
            payloadSize(0),
            maxRuns(1),
            tries(0),
            seqNum(0),
            splitDatagramInto(1),
            finishDelay(0),
            currPkt(0),
            isActive(false),
            isFirst(false),
            errors(0),
            unable(0),
            timedout(0),
            rtt(0),
            rttMax(0),
            rttMin(0),
            successful(0),
            udpTimer(NULL),
            tfEvent(this, "tf")
{
}


void TrafficGen::initialize() {
    cometos::LongScheduleModule::initialize();

    remoteDeclare(&TrafficGen::run, "run");
    remoteDeclare(&TrafficGen::reset, "reset");
    remoteDeclare(&TrafficGen::getRttStats, "getRttStats");
    remoteDeclare(&TrafficGen::getAndResetResults, "getResults");
    remoteDeclare(&TrafficGen::setStackCfg, "ssc");
    remoteDeclare(&TrafficGen::getStackCfg, "gsc");
    remoteDeclare(&TrafficGen::resetStackCfg, "rsc");


    common_init();
}


void TrafficGen::common_init() {
    LOG_DEBUG("init");

    CONFIG_NED(payloadSize);
    CONFIG_NED(mRTT);
    CONFIG_NED(rttDest);
    CONFIG_NED(mTP);
    CONFIG_NED(tpDest);
    CONFIG_NED(ratePer256s);
    CONFIG_NED(maxRuns);
    CONFIG_NED(mode);
    CONFIG_NED(splitDatagramInto);

    udp = (cometos_v6::UDPLayer*) getModule(UDP_MODULE_NAME);
    icmp = (cometos_v6::ICMPv6*) getModule(ICMP_MODULE_NAME);

    port = udp->bind(this, MT_PORT);
    icmp->registerListener(this, 1);

    if (mRTT) {
        LOG_DEBUG("messure the RTT to " << rttDest);

        isActive = true;
        longSchedule(new cometos::LongScheduleMsg(createCallback(&TrafficGen::echoTimerFired)), nextOffset());
    }

    if (mTP && !isActive && maxRuns > 0) {
        LOG_INFO("send Packets to " << tpDest << " Rate: " << ratePer256s << " Payload: " << payloadSize);
        isActive = true;
        udpTimer = new cometos::LongScheduleMsg(createCallback(&TrafficGen::udpTimerFired));
        longSchedule(udpTimer, nextOffset());
    }
}

void TrafficGen::finish() {
    recordScalar("RTT_avg", rtt);
    recordScalar("RTT_min", rttMin);
    recordScalar("RTT_max", rttMax);
    recordScalar("Err", errors);
    recordScalar("Unable", unable);
    recordScalar("TO", timedout);
    recordScalar("Succ", successful);
    recordScalar("delayMode", mode);
}



void TrafficGen::udpTimerFired(cometos::LongScheduleMsg * msg) {
    ASSERT(isActive);
    sendMessage();
    currPkt++;
    if (currPkt < maxRuns || maxRuns == 0) {
        LOG_INFO("Run Nr: " << (int)currPkt);
        longSchedule(msg, nextOffset());
    } else {
        if (msg == udpTimer) {
            udpTimer = NULL;
        }
        delete(msg);
        transferFinishedDispatch();
    }
}

void TrafficGen::echoTimerFired(cometos::LongScheduleMsg * msg) {
    delete(msg);
    ASSERT(isActive);
    LOG_INFO("ECH TO " << rttDest);
    measureRTT();
}

cometos_v6::BufferInformation* TrafficGen::getDataBuffer(uint16_t size) {
    cometos_v6::LowpanAdaptionLayer * lowpan = (cometos_v6::LowpanAdaptionLayer *) getModule(LOWPAN_MODULE_NAME);
    cometos_v6::BufferInformation* buffer = lowpan->getLowpanDataBuffer(size);
    return buffer;
}

void TrafficGen::setSeqNumAndSendMessage(uint16_t seqNum) {
    this->seqNum = seqNum;
    sendMessage();
}

void TrafficGen::sendMessage() {
    dstAddress[7] = tpDest;

    IPv6Address dst = dstAddress;

    ASSERT(udp!=NULL);

    time_ms_t ts = getCommonTime();

    uint16_t fragSize = payloadSize / splitDatagramInto;
    ASSERT(fragSize * splitDatagramInto == payloadSize);
    ASSERT(splitDatagramInto <= MT_TEST_MESSAGE_MAX_PARTS);

    for (int i = 0; i < splitDatagramInto; i++) {
        cometos_v6::BufferInformation* bi = getDataBuffer(fragSize);
        if (bi != NULL) {
            for (uint16_t i = 0; i < fragSize; i++) {
                (*bi)[i] = (i & 0xFF);
            }
            ASSERT(fragSize >= MT_TEST_MESSAGE_INFO_SIZE);

            (*bi)[0] = (seqNum >> 8) & 0xFF;
            (*bi)[1] = seqNum & 0xFF;
            (*bi)[2] = (ts >> 24) & 0xFF;
            (*bi)[3] = (ts >> 16) & 0xFF;
            (*bi)[4] = (ts >> 8) & 0xFF;
            (*bi)[5] = ts & 0xFF;
            (*bi)[6] = i;
            (*bi)[7] = splitDatagramInto;
            if (udp->sendMessage(dst, port, port, bi->getContent(), bi->getSize(), NULL)) {
                LOG_DEBUG("Sent UDP Pckt of size " << (int) fragSize << " to " << tpDest << "; seq=" << seqNum);
            } else {
                LOG_INFO("UDP Pckt of size " << (int) fragSize << " not snd to " << tpDest);
            }
        } else {
            LOG_WARN("No Buf space available");
        }
    }

    seqNum++;
}





void TrafficGen::measureRTT() {
    dstAddress[7] = rttDest;
    IPv6Address dst = dstAddress;
    time_ms_t t = getCommonTime();

    if (tries < maxRuns) {
        tries++;

        cometos_v6::BufferInformation* bi = getDataBuffer(payloadSize);

        if (bi != NULL &&
                icmp->sendEchoBI(dst,
                t,
                bi,
                this,
                &TrafficGen::receiveEchoReply))
        {
            LOG_DEBUG("ECHOREQ #" << successful);
        } else {
            if (bi == NULL) {
                LOG_ERROR("Buffer is full!");
            }
            LOG_INFO("EchoReq not send. Tries: " << (uint16_t)tries);
            unable++;
            if (tries < maxRuns) {
                LOG_DEBUG("Retry");
                longSchedule(new cometos::LongScheduleMsg(createCallback(&TrafficGen::echoTimerFired)), nextOffset());
            } else {
                LOG_DEBUG("Stop Trying");
            }
        }
    } else {
        transferFinishedDispatch();
    }
}

void TrafficGen::udpPacketReceived(const IPv6Address& src,
        uint16_t srcPort,
        uint16_t dstPort,
        const uint8_t* data,
        uint16_t length) {

    // first, inform all snooping listeners that a datagram has arrived
    for (uint8_t i = listeners.begin(); i != listeners.end(); i = listeners.next(i)) {
        listeners.get(i)->handleIncomming(src, srcPort, dstPort, data, length);
    }

    // dispatch to derived class (TrafficGenSim, PyTrafficGen) or do nothing
    // if there is no such class
    handleIncomming(src, srcPort, dstPort, data, length);
}

void TrafficGen::handleIncomming(const IPv6Address& src,
        uint16_t srcPort,
        uint16_t dstPort,
        const uint8_t* data,
        uint16_t length)
{

    if (length >= MT_TEST_MESSAGE_INFO_SIZE) {

    }
#ifndef OMNETPP
        palLed_toggle(4);
#endif
}


void TrafficGen::receiveEchoReply(uint16_t seqNum, bool success) {
    ENTER_METHOD_SILENT();
    uint16_t t = getCommonTime();
    if (success) {
        LOG_DEBUG("Rcvd Echo Rep from " << seqNum << " after "
                << (uint16_t)(t - seqNum) << " ms");

        time_ms_t tt = t - seqNum;
        if (t < seqNum) {
            uint16_t tmpMax = -1;
            tt = (tmpMax - seqNum) + t;
        }

        if (successful == 0) {
            rttMax = tt;
            rttMin = tt;
        } else {
            if (rttMax < tt) rttMax = tt;
            if (rttMin > tt) rttMin = tt;
        }

        rtt += (unsigned long)(tt);
        successful++;

        rttStats.add(tt);

    } else {
//        rttStats.incLost();
        LOG_INFO("Echo " << successful << " timed out");
        timedout++;
    }

    longSchedule(new cometos::LongScheduleMsg(createCallback(&TrafficGen::echoTimerFired)), nextOffset());
}

void TrafficGen::icmpMessageReceived(
        const IPv6Address& src,
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

time_ms_t TrafficGen::getCommonTime() {
    return NetworkTime::get();
}

void TrafficGen::transferFinished(node_t id, TgResults & results) {
    tfEvent.raiseEvent(results);
}

cometos_error_t TrafficGen::setStackCfg(StackCfg& cfg) {
    cometos_error_t result = COMETOS_SUCCESS;
    cometos_v6::UDPLayer* udp = (cometos_v6::UDPLayer*) getModule(UDP_MODULE_NAME);
    cometos_v6::IpForward* ip = (cometos_v6::IpForward*) getModule(IPFWD_MODULE_NAME);

    ASSERT(udp != nullptr);
    ASSERT(ip != nullptr);

    result = udp->setConfig(cfg.udpCfg);
    LOG_ERROR("udp: " << (int) result);
    if (result != COMETOS_SUCCESS) {
        return result;
    }
    result = ip->setConfig(cfg.ipCfg);
    LOG_ERROR("ip: " << (int) result);
    return result;
}

StackCfg TrafficGen::getStackCfg() {
    cometos_v6::UDPLayer* udp = (cometos_v6::UDPLayer*) getModule(UDP_MODULE_NAME);
    cometos_v6::IpForward* ip = (cometos_v6::IpForward*) getModule(IPFWD_MODULE_NAME);

    ASSERT(udp != nullptr);
    ASSERT(ip != nullptr);

    StackCfg cfg;
    cfg.udpCfg = udp->getConfig();
    cfg.ipCfg = ip->getConfig();

    return cfg;
}

cometos_error_t TrafficGen::resetStackCfg() {
    cometos_v6::UDPLayer* udp = (cometos_v6::UDPLayer*) getModule(UDP_MODULE_NAME);
    cometos_v6::IpForward* ip = (cometos_v6::IpForward*) getModule(IPFWD_MODULE_NAME);

    ASSERT(udp != nullptr);
    ASSERT(ip != nullptr);

    udp->resetConfig();
    ip->resetConfig();
    return COMETOS_SUCCESS;
}


TgResults & TrafficGen::getAndResetResults() {

    // send udp stats (they won't change for collection) and mac
    // stats (they would be biased by actively collecting them again)
    cometos_v6::UDPLayer * udp = (cometos_v6::UDPLayer *) getModule(UDP_MODULE_NAME);
    ASSERT(udp != NULL);
#if (not defined BOARD_python) and (defined MAC_ENABLE_STATS)
        cometos::CsmaMac * mac = (cometos::CsmaMac *) getModule(MAC_MODULE_NAME);
        ASSERT(mac != NULL);
        results.macStats = mac->getStats();
        mac->resetStats();
#endif

    results.udpStats = udp->getStats();
    udp->resetStats();

    return results;
}

void TrafficGen::transferFinishedDispatch() {
    schedule(new cometos::Message(), &TrafficGen::finishTimerFired, finishDelay);
}

void TrafficGen::finishTimerFired(cometos::Message * msg) {
    delete(msg);

//    std::cout << "tfDispatch: "<< results.udpStats.numNotSent << "|" << results.udpStats.numSent << std::endl;

    transferFinished(palId_id(), getAndResetResults());

    // reset isActive if someone has overwritten this function and marks
    // the transfer as finished by returning true
    isActive = false;
}


void TrafficGen::offsetTimerFired(StartExperimentMsg * msg) {
    getAndResetResults();
    cometos::LongScheduleMsg* lsm = new cometos::LongScheduleMsg(createCallback(msg->getMethodToExecute()));
    longSchedule(lsm, nextOffset());
    delete(msg);
    msg=NULL;
}

bool TrafficGen::run(TgConfig & cfg) {
    if (cfg.mTP) {
        currPkt = 0;
        return startExperiment(cfg, &TrafficGen::udpTimerFired);
    }

    if (cfg.mRTT) {
        tries = 0;
        return startExperiment(cfg, &TrafficGen::echoTimerFired);
    }

    return false;
}

SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> TrafficGen::getRttStats() {
    return rttStats;
}

bool TrafficGen::reset() {
    if (!isActive) {
        rttStats.reset();
        return true;
    } else {
        return false;
    }
}

bool TrafficGen::subscribeToIncommingDatagramEvent(TrafficGenListener* mtl) {
    if (listeners.full()) {
        return false;
    } else {
        listeners.push_back(mtl);
        return true;
    }
}

double TrafficGen::doublerand() {
    uint16_t rand_max = -1;
    uint16_t x = intrand(rand_max);

    double res = ((double) x) / rand_max;
    return res;
}


cometos::LongScheduleModule::longTimeOffset_t TrafficGen::nextOffset() {
    uint32_t tmp = 0;
    if (mode == IM_FIXED) {
        tmp = ((cometos::LongScheduleModule::longTimeOffset_t) 1000) * RATE_FACTOR / ratePer256s;

    } else if (mode == IM_POISSON) {
#ifdef OMNETPP
        tmp = ( ( -1000 * log( doublerand() )) * RATE_FACTOR / ratePer256s );
#else
        uint16_t rand_max = 60000;
        uint16_t x = intrand(rand_max);
        double xDbl = ((double)x+1)/((double)rand_max);
        tmp = (-1000.0 * log(xDbl) * RATE_FACTOR / ratePer256s);

        // add one to prevent a zero
        tmp += 1;
#endif

    } else if (mode == IM_UNIFORM) {
        tmp = ((cometos::LongScheduleModule::longTimeOffset_t) 1000) * RATE_FACTOR / ratePer256s;
        tmp = intrand(tmp) + tmp/2;
    }

    LOG_DEBUG("next interval: " << tmp);
    return tmp;
}



void TrafficGen::setConfig(TgConfig & cfg) {
    ASSERT(!isActive);
    payloadSize = cfg.payloadSize;
    mRTT = cfg.mRTT;
    rttDest = cfg.rttDest;
    mTP = cfg.mTP;
    tpDest = cfg.tpDest;
    ratePer256s = cfg.rate;
    mode = cfg.mode;
    maxRuns = cfg.maxRuns;
    finishDelay = cfg.finishDelay;
}

bool TrafficGen::startExperiment(TgConfig & cfg, pMtMethod m) {
    if (!isActive) {
        setConfig(cfg);
        isActive = true;
        isFirst = true;
        LOG_DEBUG("run:" << cfg.offset << "|" << payloadSize << "|" << ratePer256s);
        schedule(new StartExperimentMsg(m), &TrafficGen::offsetTimerFired, cfg.offset);
        return true;
    } else {
        return false;
    }
}



namespace cometos {
void serialize(ByteVector& buffer, const TgConfig & value) {
    serialize(buffer, value.offset);
    serialize(buffer, value.payloadSize);
    serialize(buffer, value.mRTT);
    serialize(buffer, value.rttDest);
    serialize(buffer, value.mTP);
    serialize(buffer, value.tpDest);
    serialize(buffer, value.mode);
    serialize(buffer, value.rate);
    serialize(buffer, value.maxRuns);
    serialize(buffer, value.finishDelay);
}
void unserialize(ByteVector& buffer, TgConfig & value) {
    unserialize(buffer, value.finishDelay);
    unserialize(buffer, value.maxRuns);
    unserialize(buffer, value.rate);
    unserialize(buffer, value.mode);
    unserialize(buffer, value.tpDest);
    unserialize(buffer, value.mTP);
    unserialize(buffer, value.rttDest);
    unserialize(buffer, value.mRTT);
    unserialize(buffer, value.payloadSize);
    unserialize(buffer, value.offset);
}


void serialize(ByteVector& buffer, const StackCfg & value) {
    serialize(buffer, value.udpCfg);
    serialize(buffer, value.ipCfg);
}
void unserialize(ByteVector& buffer, StackCfg & value) {
    unserialize(buffer, value.ipCfg);
    unserialize(buffer, value.udpCfg);
}


void serialize(ByteVector& buffer, const TgResults & value) {
    serialize(buffer, value.udpStats);
    serialize(buffer, value.macStats);
}
void unserialize(ByteVector& buffer, TgResults & value) {
    unserialize(buffer, value.macStats);
    unserialize(buffer, value.udpStats);
}


void serialize(ByteVector & buf, const SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> & val) {
    serialize(buf, val.len);
//    serialize(buf, val.lost);
    serialize(buf, val.sum);
    serialize(buf, val.sqrSum);
    serialize(buf, val.min);
    serialize(buf, val.max);
}

void unserialize(ByteVector & buf, SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t> & val) {
    unserialize(buf, val.max);
    unserialize(buf, val.min);
    unserialize(buf, val.sqrSum);
    unserialize(buf, val.sum);
//    unserialize(buf, val.lost);
    unserialize(buf, val.len);
}

}
