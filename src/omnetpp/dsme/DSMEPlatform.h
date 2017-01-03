/*
 * openDSME
 *
 * Implementation of the Deterministic & Synchronous Multi-channel Extension (DSME)
 * described in the IEEE 802.15.4-2015 standard
 *
 * Authors: Florian Meier <florian.meier@tuhh.de>
 *          Maximilian Koestler <maximilian.koestler@tuhh.de>
 *          Sandrina Backhauss <sandrina.backhauss@tuhh.de>
 *
 * Based on
 *          DSME Implementation for the INET Framework
 *          Tobias Luebkert <tobias.luebkert@tuhh.de>
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.
 *
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

#ifndef DSMEPLATFORM_H
#define DSMEPLATFORM_H

#include "omnetpp.h"

#include "DSMEPlatformBase.h"

namespace dsme {

class DSMEPlatform : public DSMEPlatformBase {
public:
    DSMEPlatform(const char* service_name = NULL) :
        DSMEPlatformBase(service_name),
        msgId(0)
    {
    }

    ~DSMEPlatform() {
        cancelAndDelete(ccaTimer);
        cancelAndDelete(timer);
    }

    void initialize() override;

    virtual void receiveLowerData(omnetpp::cMessage *msg) override;

    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void receiveLowerControl(omnetpp::cMessage *msg) override;

    omnetpp::SimTime symbolDuration;

    static omnetpp::simsignal_t broadcastDataSentDown;
    static omnetpp::simsignal_t unicastDataSentDown;
    static omnetpp::simsignal_t ackSentDown;
    static omnetpp::simsignal_t beaconSentDown;
    static omnetpp::simsignal_t commandSentDown;

    virtual void finish() override;

    virtual void txDone(cometos::macTxResult_t result) override;

    bool startCCA() override {
        ASSERT(!ccaTimer->isScheduled());
        channelInactive = true;
        LOG_DEBUG("CCA start");
        scheduleAt(omnetpp::simTime() + 8*symbolDuration, ccaTimer);
        return true;
    }

    void startTimer(uint32_t symbolCounterValue) override {
        omnetpp::SimTime time = symbolCounterValue*symbolDuration;
        if(timer->isScheduled()) {
            cancelEvent(timer);
        }
        scheduleAt(time, timer);
    }

    uint32_t getSymbolCounter() override {
        return omnetpp::simTime()/symbolDuration;
    }

    /**
     * Send an ACK message, delay until aTurnaRoundTime after reception_time has expired
     */
    bool sendDelayedAck(DSMEMessage *ackMsg, DSMEMessage *receivedMsg, Delegate<void(bool)> txEndCallback) override;

    /**
     * Directly send packet without delay and without CSMA
     * but keep the message (the caller has to ensure that the message is eventually released)
     * This might lead to an additional memory copy in the platform
     */
    bool sendCopyNow(DSMEMessage *msg, Delegate<void(bool)> txEndCallback) override;

    bool setChannelNumber(uint8_t k) override;

protected:
    virtual void signalNewMsg(DSMEMessage* msg) override;
    virtual void signalReleasedMsg(DSMEMessage* msg) override;

    uint16_t msgId;
    std::map<DSMEMessage*, uint16_t> msgMap;
    std::set<uint16_t> msgsActive;
};

}

#endif
