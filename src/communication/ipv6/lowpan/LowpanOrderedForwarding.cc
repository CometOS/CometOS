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
 * @author Andreas Weigel
 */

#include "LowpanOrderedForwarding.h"
#include "cometosAssert.h"
#include "palLocalTime.h"

namespace cometos_v6 {

Define_Module(LowpanOrderedForwarding);

const char* const LowpanOrderedForwarding::MODULE_NAME = "loof";
const uint8_t LowpanOrderedForwardingCfg::DEFAULT_MAX_NUM_SENT_WITHOUT_CONFIRMATION;
const uint8_t LowpanOrderedForwardingCfg::DEFAULT_BE_MAX;
const uint8_t LowpanOrderedForwardingCfg::DEFAULT_BE_MIN;
const uint8_t LowpanOrderedForwardingStats::MAX_NUM_BACKOFF_STEPS;

LowpanCongestionEvent::LowpanCongestionEvent(uint8_t signal,
                                             const IPv6Datagram& dg,
                                             const Ieee802154MacAddress& nextHop,
                                             const LowpanFragMetadata& fragMeta,
                                             const LocalDgId& dgId) :
            cometos::FSMEvent(signal),
            dg(dg),
            nextHop(nextHop),
            fragMeta(fragMeta),
            dgId(dgId)
{}


LowpanOrderedForwardingCfg::LowpanOrderedForwardingCfg(
        uint8_t beMin,
        uint8_t beMax,
        uint8_t maxNumSentWithoutConfirmation,
        bool    tagAllQueueObjects,
        bool    useCongestionFlag) :
               beMin(beMin),
               beMax(beMax),
               maxNumSentWithoutConfirmation(maxNumSentWithoutConfirmation),
               tagAllQueueObjects(tagAllQueueObjects),
               useCongestionFlag(useCongestionFlag)
{}

bool LowpanOrderedForwardingCfg::operator==(const LowpanOrderedForwardingCfg& rhs) {
    return beMin == rhs.beMin
            && beMax == rhs.beMax
            && maxNumSentWithoutConfirmation == rhs.maxNumSentWithoutConfirmation
            && tagAllQueueObjects == rhs.tagAllQueueObjects
            && useCongestionFlag == rhs.useCongestionFlag;
}

void LowpanOrderedForwardingCfg::doSerialize(cometos::ByteVector& buf) const {
    serialize(buf, beMin);
    serialize(buf, beMax);
    serialize(buf, maxNumSentWithoutConfirmation);
    serialize(buf, tagAllQueueObjects);
    serialize(buf, useCongestionFlag);
}

void LowpanOrderedForwardingCfg::doUnserialize(cometos::ByteVector& buf) {
    unserialize(buf, useCongestionFlag);
    unserialize(buf, tagAllQueueObjects);
    unserialize(buf, maxNumSentWithoutConfirmation);
    unserialize(buf, beMax);
    unserialize(buf, beMin);
}

bool LowpanOrderedForwardingCfg::isValid() {
    return beMin <= beMax
            && (beMax - beMin <= LowpanOrderedForwardingStats::MAX_NUM_BACKOFF_STEPS);
}


LowpanOrderedForwarding::LowpanOrderedForwarding(
            LowpanAdaptionLayer* lowpan,
            DgOrderedLowpanQueue* queue) :
        cometos::RemotelyConfigurableModule<LowpanOrderedForwardingCfg>(LowpanOrderedForwarding::MODULE_NAME),
        cometos::HFSM<LowpanOrderedForwarding, LowpanCongestionEvent>(&LowpanOrderedForwarding::stateInit),
        rt(NULL),
        nd(NULL),
        lowpan(nullptr),
        queue(nullptr),
        numSentWithoutConfirmation(0),
        sendingAllowed(true),
        congestionFlag(false),
        cfg(),
        probeBe(cfg.beMin),
        stats(),
        tsEnter{0}
{
    setModulePtrs(lowpan, queue);
}

void LowpanOrderedForwarding::setModulePtrs(
            LowpanAdaptionLayer* lowpan,
            DgOrderedLowpanQueue* queue)
{
    this->lowpan = lowpan;
    this->queue = queue;
    if (this->lowpan != nullptr) {
        probeTimer.setCallback((CALLBACK_MET(&LowpanOrderedForwarding::timerFired, *this)));
        stopTimer.setCallback((CALLBACK_MET(&LowpanOrderedForwarding::timerFired, *this)));
        notifyLowpan.setCallback((CALLBACK_MET(&LowpanAdaptionLayer::sendIfAllowed, *(this->lowpan))));
    }
}

LowpanOrderedForwardingStats LowpanOrderedForwarding::getStats() {
    return stats;
}

void LowpanOrderedForwarding::resetStats() {
    stats.reset();
}

bool LowpanOrderedForwarding::isBusy() {
    // there is no busy state
    return false;
}

void LowpanOrderedForwarding::applyConfig(LowpanOrderedForwardingCfg& cfg) {
    this->cfg = cfg;
}
LowpanOrderedForwardingCfg& LowpanOrderedForwarding::getActive() {
    return this->cfg;
}


void LowpanOrderedForwarding::initialize() {
    doManualInit();
}


void LowpanOrderedForwarding::doManualInit() {
    RemotelyConfigurableModule<LowpanOrderedForwardingCfg>::initialize();
    rt = (RoutingTable*) lowpan->getModule(ROUTING_TABLE_MODULE_NAME);
    ASSERT(rt != NULL);
    nd = (NeighborDiscovery*) lowpan->getModule(NEIGHBOR_DISCOVERY_MODULE_NAME);
    ASSERT(nd != NULL);

    CONFIG_NED_OBJ(cfg, beMin);
    CONFIG_NED_OBJ(cfg, beMax);
    CONFIG_NED_OBJ(cfg, maxNumSentWithoutConfirmation);
    CONFIG_NED_OBJ(cfg, tagAllQueueObjects);
    CONFIG_NED_OBJ(cfg, useCongestionFlag);
    //cometos::getCout() << "min=" << (int) cfg.beMin << "|max=" << (int) cfg.beMax << "|mnswc=" << (int) cfg.maxNumSentWithoutConfirmation << cometos::endl;
    remoteDeclare(&LowpanOrderedForwarding::getStats, "gs");
    remoteDeclare(&LowpanOrderedForwarding::resetStats, "rs");

#ifdef OMNETPP
    stateVec.setName("StateVector");
    stateVec.enable();
    stateMap[convert(&LowpanOrderedForwarding::stateInit)] = 0;
    stateMap[convert(&LowpanOrderedForwarding::stateIdle)] = 1;
    stateMap[convert(&LowpanOrderedForwarding::stateSending)] = 2;
    stateMap[convert(&LowpanOrderedForwarding::stateProbingBo)] = 3;
    stateMap[convert(&LowpanOrderedForwarding::stateProbing)] = 4;
    stateMap[convert(&LowpanOrderedForwarding::stateStopped)] = 5;
    stateMap[convert(&LowpanOrderedForwarding::stateActive)] = 6;
    stateMap[convert(&LowpanOrderedForwarding::stateStalled)] = 7;
    stateMap[convert(&LowpanOrderedForwarding::stateAlive)] = 8;
    stateNameMap[convert(&LowpanOrderedForwarding::stateInit)] = std::string("stateInit");
    stateNameMap[convert(&LowpanOrderedForwarding::stateIdle)] = std::string("stateIdle");
    stateNameMap[convert(&LowpanOrderedForwarding::stateSending)] = std::string("stateSending");;
    stateNameMap[convert(&LowpanOrderedForwarding::stateProbingBo)] = std::string("stateProbingBo");;
    stateNameMap[convert(&LowpanOrderedForwarding::stateProbing)] = std::string("stateProbing");;
    stateNameMap[convert(&LowpanOrderedForwarding::stateStopped)] = std::string("stateStopped");;
    stateNameMap[convert(&LowpanOrderedForwarding::stateActive)] = std::string("stateActive");;
    stateNameMap[convert(&LowpanOrderedForwarding::stateStalled)] = std::string("stateStalled");;
    stateNameMap[convert(&LowpanOrderedForwarding::stateAlive)] = std::string("stateAlive");;
#endif
    HFSM::run();
}

LowpanOrderedForwarding::~LowpanOrderedForwarding() {
}

cometos::fsmReturnStatus LowpanOrderedForwarding::stateInit(LowpanCongestionEvent& event) {
    switch(event.signal) {
        case LowpanCongestionEvent::INIT_SIGNAL:
            return newState(&LowpanOrderedForwarding::stateIdle);
    }

    return superState(&LowpanOrderedForwarding::top);
}

cometos::fsmReturnStatus LowpanOrderedForwarding::stateIdle(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            setTsEnter();
            congestionFlag = false;
            numSentWithoutConfirmation = 0;
            sendingAllowed = true;
            currDest.setA4(0xFFFF);
            LOG_DEBUG("Set currDest to " << cometos::hex << currDest.a1() << ":" << currDest.a2() << ":" << currDest.a3() << ":" << currDest.a4());
            return cometos::FSM_HANDLED;
        }

        case LowpanCongestionEvent::EXIT_SIGNAL: {
            stats.timeIdle += timeDiffToTsEnter();
            return cometos::FSM_HANDLED;
        }

        case LowpanCongestionEvent::SWITCH_QUEUE_OBJECT: {
            // nothing to do here
            return cometos::FSM_HANDLED;
        }

        case LowpanCongestionEvent::SENT: {
            numSentWithoutConfirmation++;

            currDest = event.nextHop;
            LOG_DEBUG("Set currDest to " << cometos::hex << currDest.a1() << ":" << currDest.a2() << ":" << currDest.a3() << ":" << currDest.a4());
            // evaluate guard
            if (checkActivityVector(event.nextHop)) {
                // nextHop has been overheard to transmit something else,
                // therefore, we enter the more defensive probe mode
                probeBe = cfg.beMin;
                return newState(&LowpanOrderedForwarding::stateProbingBo);
            } else {
                // evaluate guard
                if (numSentWithoutConfirmation <= cfg.maxNumSentWithoutConfirmation) {
                    return newState(&LowpanOrderedForwarding::stateSending);
                } else {
                    probeBe = cfg.beMin;
                    return newState(&LowpanOrderedForwarding::stateProbingBo);
                }
            }
        }
        case LowpanCongestionEvent::SENT_LAST: {
            ASSERT(event.fragMeta.isFirst);
            // no actions if we send an unfragmented dg
            return cometos::FSM_HANDLED;
        }

        case LowpanCongestionEvent::SNOOP_FOREIGN: {
            // update stored activity state in routing table
            updateActivityVector(event.fragMeta, event.nextHop);
            return cometos::FSM_HANDLED;
        }
    }
    return superState(&LowpanOrderedForwarding::top);
}

cometos::fsmReturnStatus LowpanOrderedForwarding::stateSending(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            congestionFlag = false;
            setTsEnter();
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::EXIT_SIGNAL: {
            stats.timeSending += timeDiffToTsEnter();
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::SENT: {
            numSentWithoutConfirmation++;
            const IPv6Datagram* dg = queue->getActiveObject()->getDg();
            const Ieee802154MacAddress* nh = nd->findNeighbor(dg->dst);
            bool nextHopIsDestination = (nh != NULL) && (*nh == currDest);
            if (nextHopIsDestination) {
                LOG_DEBUG("NH = DST; no interruption");
                return cometos::FSM_HANDLED;
            }

            if (numSentWithoutConfirmation <= cfg.maxNumSentWithoutConfirmation) {
                LOG_DEBUG("#sent(" << (int) numSentWithoutConfirmation << ") <= mnswc(" << (int) cfg.maxNumSentWithoutConfirmation << "); staying in send")
                return cometos::FSM_HANDLED;
            } else {
                LOG_DEBUG("#sent(" << (int) numSentWithoutConfirmation << ") > mnswc(" << (int) cfg.maxNumSentWithoutConfirmation << "); enter probingBo")
                probeBe = cfg.beMin;
                return newState(&LowpanOrderedForwarding::stateProbingBo);
            }
        }
    }
    return superState(&LowpanOrderedForwarding::stateAlive);
}


cometos::fsmReturnStatus LowpanOrderedForwarding::stateProbingBo(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            setTsEnter();
            startProbeTimer();
            congestionFlag = true;
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::EXIT_SIGNAL: {
            LOG_DEBUG("Timer cancelled");
            cometos::getScheduler().remove(probeTimer);
            stats.timeProbingBo += timeDiffToTsEnter();
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::TIMER_FIRED: {
            stats.beUsed[probeBe-cfg.beMin]++;
            return newState(&LowpanOrderedForwarding::stateProbing);
        }
    }
    return superState(&LowpanOrderedForwarding::stateStalled);
}


cometos::fsmReturnStatus LowpanOrderedForwarding::stateProbing(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            probeBe = probeBe < cfg.beMax ? probeBe + 1 : cfg.beMax;
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::SENT: {
            return newState(&LowpanOrderedForwarding::stateProbingBo);
        }
    }
    return superState(&LowpanOrderedForwarding::stateAlive);
}


cometos::fsmReturnStatus LowpanOrderedForwarding::stateStopped(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            setTsEnter();
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::EXIT_SIGNAL: {
            stats.timeStopped += timeDiffToTsEnter();
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::TIMER_FIRED: {
            probeBe = cfg.beMin;
            cometos::getScheduler().remove(stopTimer);
            return newState(&LowpanOrderedForwarding::stateProbingBo);
        }
    }
    return superState(&LowpanOrderedForwarding::stateStalled);
}


cometos::fsmReturnStatus LowpanOrderedForwarding::stateActive(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal << " queueSize=" << (int) queue->getQueueSize());
    NextHopTagStoringQueueObject* qo = queue->getActiveObject();
    const IPv6Datagram* dg = NULL;
    if (qo != NULL) {
        dg = qo->getDg();
    }
    switch(event.signal) {
        case LowpanCongestionEvent::SWITCH_QUEUE_OBJECT: {

            // if the destination is switched, we just abort anything and
            // enter idle state again and wait for next send
            return newState(&LowpanOrderedForwarding::stateIdle);
        }
        case LowpanCongestionEvent::ENQUEUE_INTERFERING_FRAME: {
            NextHopTagStoringQueueObject* qo = queue->getActiveObject();
            // check if our IP address is the larger one, in which case we 
            // abort our sending process and forward the incoming DG
            if (qo->getDg()->src > event.dg.src) {
                LOG_DEBUG("Deterring dg from " << qo->getDg()->src.str() << " in favor of incoming dg from " << event.dg.src.str());
                rotateToQueueObject(event.dgId);
#ifdef ENABLE_LOGGING
                bool idValid;
                LocalDgId dgId = queue->getActiveObject()->getDgId(idValid);
                LOG_DEBUG("New object; tag=" << dgId.tag << "|src=" << dgId.src.a4() << "|dst=" << dgId.dst.a4());
#endif
                return newState(&LowpanOrderedForwarding::stateIdle);
            } else {
                LOG_DEBUG("Keep sending ")
                return cometos::FSM_HANDLED;
            }
        }
        case LowpanCongestionEvent::SNOOP_FOREIGN: {
            // also update activity vector in active state
            updateActivityVector(event.fragMeta, event.nextHop);
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::SNOOP_NEXTHOP_FIRST: {
            ASSERT(qo != NULL);
            if (addressMatch(dg, &(event.dg))) {
                stats.snoopedNonCongestion++;
                qo->setTagOfNextHop(event.fragMeta.tag);
                numSentWithoutConfirmation = 0;
                return newState(&LowpanOrderedForwarding::stateSending);
            } else {
                LOG_DEBUG("Enter stopped");
                startStopTimer(event.fragMeta);
                return newState(&LowpanOrderedForwarding::stateStopped);
            }
        }
        case LowpanCongestionEvent::SNOOP_NEXTHOP_FIRST_CONG: {
            ASSERT(qo != NULL);
            if (addressMatch(dg, &(event.dg))) {
                stats.snoopedCongestion++;
                qo->setTagOfNextHop(event.fragMeta.tag);
                probeBe = cfg.beMin;
                return newState(&LowpanOrderedForwarding::stateProbingBo);
            } else {
                LOG_DEBUG("Enter stopped");
                startStopTimer(event.fragMeta);
                return newState(&LowpanOrderedForwarding::stateStopped);
            }
        }
        case LowpanCongestionEvent::SNOOP_NEXTHOP_N: {
            if (matchesMyActiveQueueObject(event.fragMeta, dg, qo)) {
                stats.snoopedNonCongestion++;
                numSentWithoutConfirmation = 0;
                return newState(&LowpanOrderedForwarding::stateSending);
            } else {
                LOG_DEBUG("Enter stopped");
                startStopTimer(event.fragMeta);
                return newState(&LowpanOrderedForwarding::stateStopped);
            }
        }
        case LowpanCongestionEvent::SNOOP_NEXTHOP_N_CONG: {
            if (matchesMyActiveQueueObject(event.fragMeta, dg, qo)) {
                stats.snoopedCongestion++;
                probeBe = cfg.beMin;
                return newState(&LowpanOrderedForwarding::stateProbingBo);
            } else {
                LOG_DEBUG("Enter stopped");
                startStopTimer(event.fragMeta);
                return newState(&LowpanOrderedForwarding::stateStopped);
            }
        }
        case LowpanCongestionEvent::SENT_LAST: {
            return newState(&LowpanOrderedForwarding::stateIdle);
        }
        case LowpanCongestionEvent::SEND_ABORTED: {
            return newState(&LowpanOrderedForwarding::stateIdle);
        }
    }
    return superState(&LowpanOrderedForwarding::top);
}


cometos::fsmReturnStatus LowpanOrderedForwarding::stateStalled(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            sendingAllowed = false;
            return cometos::FSM_HANDLED;
        }
        case LowpanCongestionEvent::EXIT_SIGNAL: {
            sendingAllowedCallback();
            return cometos::FSM_HANDLED;
        }
    }
    return superState(&LowpanOrderedForwarding::stateActive);
}

cometos::fsmReturnStatus LowpanOrderedForwarding::stateAlive(LowpanCongestionEvent& event) {
    LOG_DEBUG("S=" << (int) event.signal);
    switch(event.signal) {
        case LowpanCongestionEvent::ENTRY_SIGNAL: {
            sendingAllowed = true;
            return cometos::FSM_HANDLED;
        }
    }
    return superState(&LowpanOrderedForwarding::stateActive);
}


void LowpanOrderedForwarding::updateActivityVector(const LowpanFragMetadata& fragMeta,
                                                 const Ieee802154MacAddress& nextHop)
{
    rt->setActiveStateForNextHop(*(nd->reverseLookup(nextHop)), !fragMeta.isLastOfDatagram());
}

bool LowpanOrderedForwarding::checkActivityVector(const Ieee802154MacAddress& nextHop) {
    return rt->getActiveStateForNextHop(*(nd->reverseLookup(nextHop)));
}

void LowpanOrderedForwarding::timerFired() {
    LOG_DEBUG("TIMER fired");
    LowpanCongestionEvent event(LowpanCongestionEvent::TIMER_FIRED);
    dispatch(event);
}

bool LowpanOrderedForwarding::addressMatch(const IPv6Datagram* lhs, const IPv6Datagram* rhs) {
    LOG_DEBUG("Matching src=" << lhs->src.str() << "|dst=" << lhs->dst.str()
              << " with src=" << rhs->src.str() << "|dst=" << rhs->dst.str());
    return lhs->src == rhs->src
            && lhs->dst == rhs->dst;
}

inline bool LowpanOrderedForwarding::matchesMyActiveQueueObject(const LowpanFragMetadata& fHead,
                                                         const IPv6Datagram* dg,
                                                         NextHopTagStoringQueueObject* qo) {
    bool matchesMyQueueObject = false;
    if (qo->isSet()) {
        if(fHead.tag == qo->getTagOfNextHop()) {
            LOG_DEBUG("Tag match (" << fHead.tag << ")");
            matchesMyQueueObject = true;
        } else {
            LOG_DEBUG("Tag not matching (expected:" << qo->getTagOfNextHop() << " got:" << fHead.tag << ")"
                       << "sizeMatch=" << (uint16_t) (fHead.dgSize == qo->getCurrDgSize() && fHead.offset < qo->currOffset()));
            matchesMyQueueObject = false;
        }
    } else {
        // in this case we probably did not successfully snoop the first fragment,
        // we accept -- possibly wrongly -- an nth fragment with same
        // characteristics and plausible offset as coming from us
        matchesMyQueueObject = fHead.dgSize == qo->getCurrDgSize()
                     && fHead.offset < qo->currOffset();
        LOG_DEBUG("Size match (myOff=" << qo->currOffset() << " theirOff=" << fHead.offset << "):" << (uint16_t) matchesMyQueueObject);
    }
    return matchesMyQueueObject;
}

void LowpanOrderedForwarding::startStopTimer(const LowpanFragMetadata& fragMeta) {
    uint16_t expire = 0;
    if (fragMeta.fragmented) {

        // estimate, how long it will take the sender to complete the
        // fragmented datagram and schedule time accordingly
        if (!(fragMeta.dgSize >= offsetToByteSize(fragMeta.offset))) {
            ASSERT(false);
        }

        expire = fragMeta.dgSize - offsetToByteSize(fragMeta.offset);

        // we use implicit floor() here, because we implicitly add one by using the offset of
        // the CURRENT fragment
        expire = expire / fragMeta.size;
        expire = expire * avgDurationMs(lowpan->getStats()) * WAIT_PER_FRAGMENT;
        // divide by two to prevent node from stopping too long, in case intermediate fragments get lost
        expire = expire >> 1;
    } else {
        // if the snooped frame is not fragmented, just wait for one single period
        expire = avgDurationMs(lowpan->getStats()) * WAIT_PER_FRAGMENT;
    }
    LOG_DEBUG("Schedule stop timer: " << expire);
    cometos::getScheduler().replace(stopTimer, expire);
}

void LowpanOrderedForwarding::startProbeTimer() {
    uint16_t avgDuration = (uint16_t) avgDurationMs(lowpan->getStats());
    uint16_t r = intrand(1024) + 1024;

    // compensate for multiplication with e in [2^10, 2^11) by shifting right by 11 again
    uint16_t expire = (((uint32_t) avgDuration) * r * (1 << probeBe)) >> 11;
    cometos::getScheduler().replace(probeTimer, expire);
    LOG_DEBUG("Start probeTimer (avgDur=" << avgDuration <<"): " << (int) expire << "|be=" << (int) probeBe << "|r=" << r);
}

inline void LowpanOrderedForwarding::setTsEnter() {
    tsEnter = palLocalTime_get();
}

time_ms_t LowpanOrderedForwarding::timeDiffToTsEnter() {
    return palLocalTime_get() - tsEnter;
}

void LowpanOrderedForwarding::sendingAllowedCallback() {

    // schedule a task here instead of directly calling the method to
    // guarantee run to completion semantics for the FSM
    cometos::getScheduler().replace(notifyLowpan, 0);
}


cometos::fsmReturnStatus LowpanOrderedForwarding::newState(state_t next) {
    state_t curr = getState();
#ifdef OMNETPP
    LOG_DEBUG("T:" << stateNameMap[convert(curr)] << "-->"<< stateNameMap[convert(next)]);
    stateVec.record(stateMap.at(convert(next)));
#endif
    if (curr == &LowpanOrderedForwarding::stateIdle) {
        if (next == &LowpanOrderedForwarding::stateProbingBo) {
            stats.idleToProbingBo++;
        } else if (next == &LowpanOrderedForwarding::stateSending) {
            stats.idleToSending++;
        }
    } else if (curr == &LowpanOrderedForwarding::stateSending) {
        if (next == &LowpanOrderedForwarding::stateProbingBo) {
            stats.sendingToProbingBo++;
        } else if (next == &LowpanOrderedForwarding::stateStopped) {
            stats.sendingToStopped++;
        }
    } else if (curr == &LowpanOrderedForwarding::stateProbing || curr == &LowpanOrderedForwarding::stateProbingBo) {
        if (next == &LowpanOrderedForwarding::stateProbingBo) {
            stats.probingToProbingBo++;
        } else if (next == &LowpanOrderedForwarding::stateSending) {
            stats.probingToSending++;
        } else if (next == &LowpanOrderedForwarding::stateStopped) {
            stats.probingToStopped++;
        }
    } else if (curr == &LowpanOrderedForwarding::stateStopped) {
        if (next == &LowpanOrderedForwarding::stateSending) {
            stats.stoppedToSending++;
        } else if (next == &LowpanOrderedForwarding::stateProbingBo) {
            stats.stoppedToProbingBo++;
        }
    }

    return transition(next);
}

void LowpanOrderedForwarding::rotateToQueueObject(const LocalDgId& dgId) {
    NextHopTagStoringQueueObject* old = queue->getActiveObject();
    NextHopTagStoringQueueObject* curr = queue->getActiveObject();
    do {
        curr = queue->getActiveObject();
        // if we found the corresponding queue object and pushed it to front,
        // we are done
        if (curr->belongsTo(dgId.src, dgId.dst, dgId.tag, dgId.size)) {
            return;
        }
        queue->pushBack();
    } while(old != curr);


    LOG_WARN("No corresponding queue object found");
}

#ifdef OMNETPP
std::string LowpanOrderedForwarding::convert(state_t ptr) {
    char buf[sizeof(ptr)];
    memcpy(&buf,&ptr,sizeof(ptr));
    return std::string(buf,sizeof(ptr));
}
#endif

void LowpanOrderedForwarding::doFinish(){
    RECORD_NAMED_SCALAR("idleToProbingBo", stats.idleToProbingBo);
    RECORD_NAMED_SCALAR("idleToSending", stats.idleToSending);
    RECORD_NAMED_SCALAR("probingToSending", stats.probingToSending);
    RECORD_NAMED_SCALAR("probingBoToProbing", stats.probingToProbingBo);
    RECORD_NAMED_SCALAR("probingToStopped", stats.probingToStopped);
    RECORD_NAMED_SCALAR("stoppedToSending", stats.stoppedToSending);
    RECORD_NAMED_SCALAR("stoppedToProbingBo", stats.stoppedToProbingBo);
    RECORD_NAMED_SCALAR("sendingToProbingBo", stats.sendingToProbingBo);
    RECORD_NAMED_SCALAR("sendingToStopped", stats.sendingToStopped);
    RECORD_NAMED_SCALAR("snoopedCongestion", stats.snoopedCongestion);
    RECORD_NAMED_SCALAR("snoopedNonCongestion", stats.snoopedNonCongestion);
    RECORD_NAMED_SCALAR("timeIdle", stats.timeIdle);
    RECORD_NAMED_SCALAR("timeProbing", stats.timeProbingBo);
    RECORD_NAMED_SCALAR("timeSending", stats.timeSending);
    RECORD_NAMED_SCALAR("timeStopped", stats.timeStopped);
    RECORD_SCALAR_ARRAY("numProbingWithBe", LowpanOrderedForwardingStats::MAX_NUM_BACKOFF_STEPS, stats.beUsed, cfg.beMin);
}

bool LowpanOrderedForwarding::handleSendingAllowed() {
    return sendingAllowed;
}

bool LowpanOrderedForwarding::handleGetCongestionStatus() {
    ENTER_METHOD_SILENT();
    if (cfg.useCongestionFlag) {
        return congestionFlag;
    } else {
        return false;
    }
}

void LowpanOrderedForwarding::handleAbortFrame() {
    ENTER_METHOD_SILENT();
    LowpanCongestionEvent event;
    event.signal = LowpanCongestionEvent::SEND_ABORTED;
    dispatch(event);
}

void LowpanOrderedForwarding::handleSwitchedQueueObject() {
    ENTER_METHOD_SILENT();
    LowpanCongestionEvent event(LowpanCongestionEvent::SWITCH_QUEUE_OBJECT);
    dispatch(event);
}

void LowpanOrderedForwarding::handleSentFrame(const LowpanFragMetadata& fHead,
                                            const Ieee802154MacAddress& dst) {
    ENTER_METHOD_SILENT();
    IPv6Datagram dummyDg;
    LowpanCongestionEvent event(LowpanCongestionEvent::SENT_LAST, dummyDg, dst, fHead);
    if (fHead.isLastOfDatagram()) {
        event.signal = LowpanCongestionEvent::SENT_LAST;
    } else {
        event.signal = LowpanCongestionEvent::SENT;
    }
    dispatch(event);
}

void LowpanOrderedForwarding::handleSnoopedFrame(const IPv6Datagram& dg,
                                               const Ieee802154MacAddress& src,
                                               const LowpanFragMetadata& fHead) {
    ENTER_METHOD_SILENT();
    LowpanCongestionEvent event(LowpanCongestionEvent::SNOOP_NEXTHOP_FIRST, dg, src, fHead);
    palLed_toggle(2);
    LOG_DEBUG("snooped frame from src=" << cometos::hex
            << src.a1() << ":" << src.a2() << ":" << src.a3() << ":" << src.a4() << "; currDest="
            << currDest.a1() << ":" << currDest.a2() << ":" << currDest.a3() << ":" << currDest.a4());
    if (currDest == src) {
        if (fHead.isFirst) {
            if (fHead.congestionFlag) {
                event.signal = LowpanCongestionEvent::SNOOP_NEXTHOP_FIRST_CONG;
            } else {
                event.signal = LowpanCongestionEvent::SNOOP_NEXTHOP_FIRST;
            }
        } else {
            if (fHead.congestionFlag) {
                event.signal = LowpanCongestionEvent::SNOOP_NEXTHOP_N_CONG;
            } else {
                event.signal = LowpanCongestionEvent::SNOOP_NEXTHOP_N;
            }
        }
    } else {
        event.signal = LowpanCongestionEvent::SNOOP_FOREIGN;
    }

    dispatch(event);
}

void LowpanOrderedForwarding::handleEnqueueFrame(const IPv6Datagram& dg,
                                                const LocalDgId& dgId) {
    ENTER_METHOD_SILENT();
    LowpanCongestionEvent event(LowpanCongestionEvent::ENQUEUE_INTERFERING_FRAME,
                                dg,
                                dgId.src,
                                LowpanFragMetadata(),
                                dgId);
    if (currDest == dgId.src) {
        dispatch(event);
    }
}

} // namespace cometos_v6

namespace cometos {
    void serialize(ByteVector& buf, const cometos_v6::LowpanOrderedForwardingStats& val) {
        serialize(buf, val.beUsed);
        serialize(buf, val.sendingToStopped);
        serialize(buf, val.probingToStopped);
        serialize(buf, val.probingToProbingBo);
        serialize(buf, val.probingToSending);
        serialize(buf, val.stoppedToSending);
        serialize(buf, val.stoppedToProbingBo);
        serialize(buf, val.idleToProbingBo);
        serialize(buf, val.idleToSending);
        serialize(buf, val.sendingToProbingBo);
        serialize(buf, val.timeIdle);
        serialize(buf, val.timeProbingBo);
        serialize(buf, val.timeSending);
        serialize(buf, val.timeStopped);
        serialize(buf, val.snoopedCongestion);
        serialize(buf, val.snoopedNonCongestion);
    }

    void unserialize(ByteVector& buf, cometos_v6::LowpanOrderedForwardingStats& val) {
        unserialize(buf, val.snoopedNonCongestion);
        unserialize(buf, val.snoopedCongestion);
        unserialize(buf, val.timeStopped);
        unserialize(buf, val.timeSending);
        unserialize(buf, val.timeProbingBo);
        unserialize(buf, val.timeIdle);
        unserialize(buf, val.sendingToProbingBo);
        unserialize(buf, val.idleToSending);
        unserialize(buf, val.idleToProbingBo);
        unserialize(buf, val.stoppedToProbingBo);
        unserialize(buf, val.stoppedToSending);
        unserialize(buf, val.probingToSending);
        unserialize(buf, val.probingToProbingBo);
        unserialize(buf, val.probingToStopped);
        unserialize(buf, val.sendingToStopped);
        unserialize(buf, val.beUsed);
    }
}


