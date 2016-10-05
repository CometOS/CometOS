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
 * @author Andreas Weigel, Geogin Varghese
 */

#ifndef LOWPANADAPTATIONLAYERSTRUCTURES_H_
#define LOWPANADAPTATIONLAYERSTRUCTURES_H_

#include "IPv6Request.h"
#include "RetransmissionList.h"
#include "PersistableConfig.h"
#include "LowpanQueue.h"
/*MACROS---------------------------------------------------------------------*/

#ifndef OMNETPP
#define LAL_SCALAR(x)           uint16_t x
#else
#define LAL_SCALAR(x)           uint32_t x
#endif
#define LAL_SCALAR_INI(x, y)    x(y)
#define LAL_SCALAR_SET(x, y)    stats.x = (y)
#define LAL_SCALAR_ADD(x, y)    stats.x += (y)
#define LAL_SCALAR_ADD_F(x, y)  stats.x += (y)
#define LAL_SCALAR_INC(x)       stats.x++

#ifdef OMNETPP
#define LAL_SCALAR_REC(y, x)    recordScalar(y, stats.x)

#define LAL_VECTOR(x)           omnetpp::cOutVector x
#define LAL_VECTOR_REC(x, y)    x.record(y)
#define LAL_VECTOR_INI(x)       x.setName(#x);x.enable()

#else

#define LAL_SCALAR_REC(y, x)

#define LAL_VECTOR(x)
#define LAL_VECTOR_REC(x, y)
#define LAL_VECTOR_INI(x)

#endif

#define LFFR_NUMBER_OF_IP_ENTRIES 10

namespace cometos_v6 {
typedef IpRetransmissionList<(uint8_t) LFFR_NUMBER_OF_IP_ENTRIES> retransmissionList;

typedef uint8_t delayMode_t;
enum {
    DM_NONE = 0,
    DM_RATE = 1,
    DM_OWN = 2,
    DM_ADAPTIVE = 3,
    DM_MAX_VALUE = DM_ADAPTIVE
};

/**
 * Factored out of LowpanConfig to prevent
 */
struct LowpanConfigConstants {
    enum QueueType {
        QT_FIFO,
        QT_DG_ORDERED
    };

    enum CongestionControlType {
        CCT_NONE,
        CCT_DG_ORDERED
    };

    static const uint8_t DEFAULT_MIN_DELAY_NOMINATOR = 3;
    static const uint8_t DEFAULT_MIN_DELAY_DENOMINATOR = 2;
    static const uint8_t DEFAULT_QUEUE_SWITCH_AFTER = 3;
};

/**
 * 6LoWPAN configuration object (rate restriction mode, macControl)
 */
struct LowpanConfig : public cometos::PersistableConfig {

    LowpanConfig(macControlMode_t mcm = LOWPAN_MACCONTROL_DEFAULT,
                 delayMode_t dm = DM_NONE,
                 uint8_t queueType = LowpanConfigConstants::QT_FIFO,
                 uint8_t minDelayNominator = LowpanConfigConstants::DEFAULT_MIN_DELAY_NOMINATOR,
                 uint8_t minDelayDenominator = LowpanConfigConstants::DEFAULT_MIN_DELAY_DENOMINATOR,
                 bool enableLFFR = false,
                 uint8_t numReassemblyHandlers = LOWPAN_SET_ASSEMBLY_ENTRIES,
                 uint16_t bufferSize = LOWPAN_SET_BUFFER_SIZE,
                 uint8_t numBufferHandlers = LOWPAN_SET_BUFFER_ENTRIES,
                 uint8_t numDirectDatagramHandlers = LOWPAN_SET_DF_PACKETS,
#ifdef LOWPAN_ENABLE_DIRECT_FORWARDING
                 bool enableDirectFwd = true,
                 uint8_t queueSize = LOWPAN_SET_BUFFER_ENTRIES,
#else
                 bool enableDirectFwd = false,
                 uint8_t queueSize = LOWPAN_SET_ASSEMBLY_ENTRIES + LOWPAN_ADDITIONAL_QUEUE_SIZE_ASSEMBLY,
#endif
                 uint8_t numIndicationMsgs = LOWPAN_MAX_CONTENTS,
                 uint8_t congestionControlType = LowpanConfigConstants::CCT_NONE,
                 uint16_t timeoutMs = DEFAULT_LOWPAN_TIMEOUT,
                 uint8_t queueSwitchAfter = LowpanConfigConstants::DEFAULT_QUEUE_SWITCH_AFTER,
                 bool pushBackStaleObjects = false,
                 bool useRateTimerForQueueSwitch = false) :
        macRetryControlMode(mcm),
        delayMode(dm),
        queueType(queueType),
        minDelayNominator(minDelayNominator),
        minDelayDenominator(minDelayDenominator),
        enableLFFR(enableLFFR),
        numReassemblyHandlers(numReassemblyHandlers),
        bufferSize(bufferSize),
        numBufferHandlers(numBufferHandlers),
        numDirectDatagramHandlers(numDirectDatagramHandlers),
        enableDirectFwd(enableDirectFwd),
        numIndicationMsgs(numIndicationMsgs),
        congestionControlType(congestionControlType),
        timeoutMs(timeoutMs),
        queueSwitchAfter(queueSwitchAfter),
        pushBackStaleObjects(pushBackStaleObjects),
        queueSize(queueSize),
        useRateTimerForQueueSwitch(useRateTimerForQueueSwitch)
    {}

    virtual void doSerialize(cometos::ByteVector& buf) const;

    virtual void doUnserialize(cometos::ByteVector& buf);

    bool isValid();

    macControlMode_t macRetryControlMode;
    delayMode_t delayMode;
    uint8_t queueType;
    uint8_t minDelayNominator;
    uint8_t minDelayDenominator;
    bool enableLFFR;
    uint8_t numReassemblyHandlers;
    uint16_t bufferSize;
    uint8_t numBufferHandlers;
    uint8_t numDirectDatagramHandlers;
    bool enableDirectFwd;
    uint8_t numIndicationMsgs;
    uint8_t congestionControlType;
    uint16_t timeoutMs;
    uint8_t queueSwitchAfter;
    bool pushBackStaleObjects;
    uint8_t queueSize;
    bool useRateTimerForQueueSwitch;

    bool operator==(const LowpanConfig & rhs);
};


/**
 * For collecting scalar statistics
 */
struct LowpanAdaptionLayerStats {
    static const uint16_t DURATION_FACTOR = 256;
    static const uint16_t DURATION_FACTOR_SHIFT = 8;

    LowpanAdaptionLayerStats() :
        LAL_SCALAR_INI(dropped_Invalid, 0),
        LAL_SCALAR_INI(dropped_QueueFull, 0),
        LAL_SCALAR_INI(dropped_MacUnsuccessful, 0),
        LAL_SCALAR_INI(dropped_FollowingUnsuccessful, 0),
        LAL_SCALAR_INI(dropped_BufferFull, 0),
        LAL_SCALAR_INI(dropped_BufferOutOfHandlers, 0),
        LAL_SCALAR_INI(dropped_AssemblyBufferFull, 0),
        LAL_SCALAR_INI(dropped_TinyBufferFull, 0),
        LAL_SCALAR_INI(dropped_InvalidOrder, 0),
        LAL_SCALAR_INI(dropped_InvalidOrderMissing, 0),
        LAL_SCALAR_INI(dropped_OutOfMessages, 0),
        LAL_SCALAR_INI(dropped_Timeout, 0),
        LAL_SCALAR_INI(dropped_NoRoute, 0),
        LAL_SCALAR_INI(dropped_HopsLeft, 0),
        LAL_SCALAR_INI(avgDuration, 0),
        LAL_SCALAR_INI(sentFrames, 0),
        LAL_SCALAR_INI(sentPackets, 0),
        LAL_SCALAR_INI(receivedFrames, 0),
        LAL_SCALAR_INI(receivedPackets, 0),
        LAL_SCALAR_INI(timesQueueEmpty, 0),
        LAL_SCALAR_INI(objectsPushedBack, 0),
        LAL_SCALAR_INI(timesQueueRecovered, 0),
        LAL_SCALAR_INI(timesQueueNotRecovered, 0)
    {}

    void reset() {
        dropped_Invalid = 0;
        dropped_QueueFull = 0;
        dropped_MacUnsuccessful = 0;
        dropped_FollowingUnsuccessful = 0;
        dropped_BufferFull = 0;
        dropped_BufferOutOfHandlers = 0;
        dropped_AssemblyBufferFull = 0;
        dropped_TinyBufferFull = 0;
        dropped_InvalidOrder = 0;
        dropped_InvalidOrderMissing = 0;
        dropped_OutOfMessages = 0;
        dropped_Timeout = 0;
        dropped_NoRoute = 0;
        dropped_HopsLeft = 0;
        avgDuration = 0;
        sentFrames = 0;
        sentPackets = 0;
        receivedFrames = 0;
        receivedPackets = 0;
        timesQueueEmpty = 0;
        objectsPushedBack = 0;
        timesQueueRecovered = 0;
        timesQueueNotRecovered = 0;
    }

    LAL_SCALAR(dropped_Invalid);
    LAL_SCALAR(dropped_QueueFull);
    LAL_SCALAR(dropped_MacUnsuccessful);
    LAL_SCALAR(dropped_FollowingUnsuccessful);
    LAL_SCALAR(dropped_BufferFull);
    LAL_SCALAR(dropped_BufferOutOfHandlers);
    LAL_SCALAR(dropped_AssemblyBufferFull);
    LAL_SCALAR(dropped_TinyBufferFull);
    LAL_SCALAR(dropped_InvalidOrder);
    LAL_SCALAR(dropped_InvalidOrderMissing);
    LAL_SCALAR(dropped_OutOfMessages);
    LAL_SCALAR(dropped_Timeout);
    LAL_SCALAR(dropped_NoRoute);
    LAL_SCALAR(dropped_HopsLeft);
    LAL_SCALAR(avgDuration);
    LAL_SCALAR(sentFrames);
    LAL_SCALAR(sentPackets);
    LAL_SCALAR(receivedFrames);
    LAL_SCALAR(receivedPackets);
    LAL_SCALAR(timesQueueEmpty);
    LAL_SCALAR(objectsPushedBack);
    LAL_SCALAR(timesQueueRecovered);
    LAL_SCALAR(timesQueueNotRecovered);
};

uint32_t avgDurationMs(const LowpanAdaptionLayerStats& lals);

struct IPv6Request_Content_t {
    IPv6Request         request;
    BufferInformation*  buf;
    IPv6Request_Content_t():
        buf(NULL) {}

    bool mapsTo(IPv6Request & req) {
        return &req == &request;
    }
};

struct IPv6DatagramInformation {
    IPv6Datagram* datagram;
    BufferInformation*  buf;
    LlRxInfo * rxInfo;
    IPv6DatagramInformation():
        datagram(NULL),
        buf(NULL),
        rxInfo(NULL)
        {}

    bool isValid() {
        return ((datagram != NULL) &&
                (buf != NULL));
    }
    void setData(IPv6Datagram* datagram_i,
            BufferInformation*  buf_i,
            LlRxInfo * rxInfo_i){
        datagram = datagram_i;
        buf = buf_i;
        rxInfo =rxInfo_i;
    }
    void free(){
        if(datagram != NULL) {
            delete datagram;
        }
        if(buf != NULL) {
            buf->free();
        }
        if(rxInfo != NULL) {
            delete rxInfo;
        }
    }
};

}



#endif /* LOWPANADAPTATIONLAYERSTRUCTURES_H_ */
