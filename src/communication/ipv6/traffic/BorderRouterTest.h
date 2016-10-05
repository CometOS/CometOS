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
 * @author Martin Ringwelski
 */

#ifndef BORDERROUTERTEST_H_
#define BORDERROUTERTEST_H_

#include "cometos.h"
#include "UDPLayer.h"
#include "ICMPv6.h"
#include "LongScheduleModule.h"
#include "Statistics.h"
#include "LowpanAdaptionLayer.h"
#include "MacStats.h"

#ifndef BASESTATION_ADDR
#define BASESTATION_ADDR 0
#endif



struct TgResults {
    TgResults() {}

    cometos_v6::UdpLayerStats udpStats;
    cometos_v6::LowpanAdaptionLayerStats lowpanStats;
    cometos::MacStats macStats;
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




class BorderRouterTest: public cometos::LongScheduleModule, public cometos_v6::UDPListener {
public:
    BorderRouterTest(const char * service_name = NULL);
    ~BorderRouterTest() {}

    virtual void initialize();

    virtual void finish();

    virtual void udpPacketReceived(const IPv6Address& src,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length);

//    MtResults results;

protected:

    cometos_v6::UDPLayer*   udp;

    uint16_t port;

};



#endif
