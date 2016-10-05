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

#ifndef DG_ORDERED_FORWARDING_CC_H_
#define DG_ORDERED_FORWARDING_CC_H_

#include "HFSM.h"
#include "LocalCongestionAvoider.h"
#include "LowpanAdaptionLayer.h"
#include "RoutingTable.h"
#include "NeighborDiscovery.h"
#include "DgOrderedLowpanQueue.h"
#include "ParameterStore.h"

#ifdef OMNETPP
#include <map>
#include <string>
#endif

namespace cometos_v6 {

struct LowpanOrderedForwardingCfg : public cometos::PersistableConfig {
    static const uint8_t DEFAULT_MAX_NUM_SENT_WITHOUT_CONFIRMATION = 1;
    static const uint8_t DEFAULT_BE_MAX = 5;
    static const uint8_t DEFAULT_BE_MIN = 1;

    explicit LowpanOrderedForwardingCfg(
           uint8_t beMin = DEFAULT_BE_MIN,
           uint8_t beMax = DEFAULT_BE_MAX,
           uint8_t maxNumSentWithoutConfirmation = DEFAULT_MAX_NUM_SENT_WITHOUT_CONFIRMATION,
           bool tagAllQueueObjects = false,
           bool useCongestionFlag = true);

    bool operator==(const LowpanOrderedForwardingCfg& rhs);

    void doSerialize(cometos::ByteVector&) const;

    void doUnserialize(cometos::ByteVector&);

    bool isValid();

    uint8_t beMin;
    uint8_t beMax;
    uint8_t maxNumSentWithoutConfirmation;
    bool tagAllQueueObjects;
    bool useCongestionFlag;
};

struct LowpanOrderedForwardingStats {
    static const uint8_t MAX_NUM_BACKOFF_STEPS = 5;

    LowpanOrderedForwardingStats()
    {
        for (uint8_t i = 0; i < MAX_NUM_BACKOFF_STEPS; i++) {
            beUsed.pushBack(0);
        }
        reset();
    }

    void reset() {
        probingToProbingBo = 0;
        probingToStopped = 0;
        probingToSending = 0;
        stoppedToSending = 0;
        stoppedToProbingBo = 0;
        sendingToStopped = 0;
        sendingToProbingBo = 0;
        idleToProbingBo = 0;
        idleToSending = 0;
        timeIdle = 0;
        timeStopped = 0;
        timeProbingBo = 0;
        timeSending = 0;
        snoopedCongestion = 0;
        snoopedNonCongestion = 0;
        for (uint8_t i = 0; i < MAX_NUM_BACKOFF_STEPS; i++) {
            beUsed[i] = 0;
        }
    }

    // counter for state transitions
    uint32_t probingToProbingBo;
    uint32_t probingToStopped;
    uint32_t probingToSending;
    uint32_t sendingToStopped;
    uint32_t sendingToProbingBo;
    uint32_t idleToProbingBo;
    uint32_t idleToSending;
    uint32_t stoppedToSending;
    uint32_t stoppedToProbingBo;
    uint32_t snoopedCongestion;
    uint32_t snoopedNonCongestion;

    /** milliseconds spent in idle mode */
    uint32_t timeIdle;

    /** milliseconds spent in stopped mode */
    uint32_t timeStopped;

    /** milliseconds spent in probingBo mode */
    uint32_t timeProbingBo;

    /** milliseconds spent in sending mode */
    uint32_t timeSending;

    /** counting the occurrences of used backoff exponents for FIRED backoffs
     *  (not those that where aborted due to a positive snoop */
    cometos::Vector<uint16_t, MAX_NUM_BACKOFF_STEPS> beUsed;
};



struct LowpanCongestionEvent : public cometos::FSMEvent {
    enum  {
        SENT = cometos::FSMEvent::USER_SIGNAL_START,
        SENT_LAST,
        SNOOP_NEXTHOP_FIRST,
        SNOOP_NEXTHOP_FIRST_CONG,
        SNOOP_NEXTHOP_N,
        SNOOP_NEXTHOP_N_CONG,
        TIMER_FIRED,
        SWITCH_QUEUE_OBJECT,
        SNOOP_FOREIGN,
        SEND_ABORTED,
        ENQUEUE_INTERFERING_FRAME
    };

    LowpanCongestionEvent(uint8_t signal = SENT,
                  const IPv6Datagram& dg = IPv6Datagram(),
                  const Ieee802154MacAddress& src = Ieee802154MacAddress(),
                  const LowpanFragMetadata& fragMeta = LowpanFragMetadata(),
                  const LocalDgId& dgId = LocalDgId((uint16_t)0, (uint16_t)0, 0, 0));

    const IPv6Datagram& dg;
    const Ieee802154MacAddress& nextHop;
    const LowpanFragMetadata& fragMeta;
    const LocalDgId& dgId;
};



class LowpanOrderedForwarding : public cometos::RemotelyConfigurableModule<LowpanOrderedForwardingCfg>,
                                public LocalCongestionAvoider,
                                public cometos::HFSM<LowpanOrderedForwarding, LowpanCongestionEvent>  {
public:
    static const char* const MODULE_NAME;

    LowpanOrderedForwarding(LowpanAdaptionLayer* lowpan = nullptr,
                            DgOrderedLowpanQueue* queue = nullptr);

    virtual ~LowpanOrderedForwarding();

    virtual void initialize();


    /** If module pointers to 6LoWPAN and 6LoWPAN ordered queue cannot be
     * provided at construction time.
     *
     * @param[in] lowpan pointer to 6LoWPAN layer
     * @param[in] queue  pointer to 6LoWPAN ordered forwarding queue
     */
    void setModulePtrs(LowpanAdaptionLayer* lowpan,
                           DgOrderedLowpanQueue* queue);

    LowpanOrderedForwardingStats getStats();

    void resetStats();

    virtual bool isBusy();
    virtual void applyConfig(LowpanOrderedForwardingCfg& cfg);
    virtual LowpanOrderedForwardingCfg& getActive();

//    cometos_error_t setConfig(LowpanOrderedForwardingCfg & cfg);
//
//    LowpanOrderedForwardingCfg getConfig();
//
//    LowpanOrderedForwardingCfg getActiveConfig();
//
//    cometos_error_t resetConfig();

    /// state handler functions, map directly to HFSM
    cometos::fsmReturnStatus stateInit(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateIdle(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateSending(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateProbingBo(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateProbing(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateStopped(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateActive(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateStalled(LowpanCongestionEvent& event);
    cometos::fsmReturnStatus stateAlive(LowpanCongestionEvent& event);


private:
    static const uint8_t WAIT_PER_FRAGMENT = 3;

    void sendingAllowedCallback();

    void updateActivityVector(const LowpanFragMetadata& fragMeta,
                              const Ieee802154MacAddress& nextHop);

    bool checkActivityVector(const Ieee802154MacAddress& nextHop);

    cometos::fsmReturnStatus newState(state_t state);

    void timerFired();

    void rotateToQueueObject(const LocalDgId& dgId);

    bool addressMatch(const IPv6Datagram* lhs, const IPv6Datagram* rhs);

    void startStopTimer(const LowpanFragMetadata& fragMeta);

    void startProbeTimer();

    void setTsEnter();

    bool matchesMyActiveQueueObject(const LowpanFragMetadata& fHead,
                                    const IPv6Datagram* dg,
                                    NextHopTagStoringQueueObject* qo);

    time_ms_t timeDiffToTsEnter();

    virtual void doManualInit();

    virtual void doFinish();

    virtual bool handleSendingAllowed();

    virtual bool handleGetCongestionStatus();

    virtual void handleAbortFrame();

    virtual void handleSwitchedQueueObject();

    virtual void handleSentFrame(const LowpanFragMetadata& fHead,
                                 const Ieee802154MacAddress& dst);

    virtual void handleSnoopedFrame(const IPv6Datagram& dg,
                                    const Ieee802154MacAddress& src,
                                    const LowpanFragMetadata& fHead);

    virtual void handleEnqueueFrame(const IPv6Datagram& dg,
                                    const LocalDgId& dgId);


    /** pointer to IPv6 routing table */
    RoutingTable* rt;

    /** pointer to IPv6 neighbor discovery */
    NeighborDiscovery* nd;

    /** pointer to 6LoWPAN adaption layer */
    LowpanAdaptionLayer* lowpan;

    /** pointer to 6LoWPAN ordered forwarding queue */
    DgOrderedLowpanQueue* queue;

    /** number of frames sent since last positive forward snoop */
    uint8_t numSentWithoutConfirmation:6;

    /** set, if sending is allowed by ordered forwarding */
    uint8_t sendingAllowed:1;

    /** set, if we enter any probing state */
    uint8_t congestionFlag:1;

    LowpanOrderedForwardingCfg cfg;

    /** current backoff exponent for probing */
    uint8_t probeBe;

    /** current's datagram next-hop destination */
    Ieee802154MacAddress currDest;

    /** timer for probingBo state */
    cometos::CallbackTask probeTimer;

    /** timer for stopped state */
    cometos::CallbackTask stopTimer;

    /** "timer" to schedule a task to notify the 6LoWPAN layer that sending is allowed */
    cometos::CallbackTask notifyLowpan;


    LowpanOrderedForwardingStats stats;

    time_ms_t tsEnter;

#ifdef OMNETPP
    std::string convert(state_t ptr);
    std::map<std::string, int> stateMap;
    std::map<std::string, std::string> stateNameMap;
    omnetpp::cOutVector stateVec;
#endif
};

} /* namespace cometos_v6 */

namespace cometos {
    void serialize(ByteVector& buf, const cometos_v6::LowpanOrderedForwardingStats& val);
    void unserialize(ByteVector& buf, cometos_v6::LowpanOrderedForwardingStats& val);
}

#endif /* DELAYPROVIDER802154_H_ */
