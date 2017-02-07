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

#ifndef DSMEPLATFORM_H
#define DSMEPLATFORM_H

#include "DSMEPlatformBase.h"
#include "Module.h"
#include "DataRequest.h"
#include "DataIndication.h"

#include "../tosMac/tosMsgReplace.h"

#include <stdint.h>

namespace dsme {

class DSMEPlatform : public DSMEPlatformBase {
public:
    DSMEPlatform(const char* service_name = NULL);

    static DSMEPlatform* instance;

    virtual ~DSMEPlatform() {
    }

    virtual void initialize() override;

    bool startCCA() override;

    void startTimer(uint32_t symbolCounterValue) override;

    uint32_t getSymbolCounter() override;

    /**
     * Directly send packet without delay and without CSMA
     * but keep the message (the caller has to ensure that the message is eventually released)
     * This might lead to an additional memory copy in the platform
     */
    bool prepareSendingCopy(IDSMEMessage *msg, Delegate<void(bool)> txEndCallback) override;

    bool sendNow() override;

    void abortPreparedTransmission() override;

    /**
     * Send an ACK message, delay until aTurnaRoundTime after reception_time has expired
     */
    bool sendDelayedAck(IDSMEMessage *ackMsg, IDSMEMessage *receivedMsg, Delegate<void(bool)> txEndCallback);

    bool setChannelNumber(uint8_t k) override;

    DSMELayer& getDSME() {
        return dsme;
    }

    static uint8_t state;

    enum {
        STATE_READY = 0, STATE_CCA_WAIT = 1, STATE_SEND = 2,
    };

    static Delegate<void(bool)> txEndCallback;

    static message_t* receive_phy(message_t* msg);

protected:
    static uint32_t getSFDTimestamp();

    static uint8_t data[127];

    static mac_ackCfg_t ackCfg;
    static mac_backoffCfg_t backoffCfg;

    static message_t phy_msg;

    uint8_t channel;
};

}

#endif
