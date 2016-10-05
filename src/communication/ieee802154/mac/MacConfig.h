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

#ifndef MACCONFIG_H_
#define MACCONFIG_H_

#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "PersistableConfig.h"
#include "Vector.h"
#include "mac_constants.h"
#include "mac_interface.h"

namespace cometos {
struct MacConfig : public PersistableConfig {
    MacConfig() :
        ccaMode(MAC_DEFAULT_CCA_MODE),
        ccaThreshold(MAC_DEFAULT_CCA_THRESHOLD),
        nwkId(MAC_DEFAULT_NWK_ID),
        channel(MAC_DEFAULT_CHANNEL),
        minBE(MAC_DEFAULT_MIN_BE),
        maxBE(MAC_DEFAULT_MAX_BE),
        maxFrameRetries(MAC_DEFAULT_FRAME_RETRIES),
        maxBackoffRetries(MAC_DEFAULT_MAX_CCA_RETRIES),
        ackWaitDuration(MAC_DEFAULT_ACK_WAIT_DURATION),
        unitBackoff(MAC_DEFAULT_UNIT_BACKOFF),
        macMode(MAC_MODE_CCA | MAC_MODE_AUTO_ACK | MAC_MODE_BACKOFF),
        txPower(MAC_DEFAULT_TX_POWER_LVL),
        promiscuousMode(false)
    {
    }


    mac_ccaMode_t ccaMode;
    mac_dbm_t ccaThreshold;
    mac_networkId_t nwkId;
    mac_channel_t channel;
    uint8_t minBE;
    uint8_t maxBE;
    uint8_t maxFrameRetries;
    uint8_t maxBackoffRetries;
    uint16_t ackWaitDuration;
    uint16_t unitBackoff;
    mac_txMode_t macMode;
    mac_power_t txPower;
    bool promiscuousMode;

    bool operator==(const MacConfig & rhs) {
        return ccaMode == rhs.ccaMode
                && ccaThreshold == rhs.ccaThreshold
                && nwkId == rhs.nwkId
                && channel == rhs.channel
                && minBE == rhs.minBE
                && maxBE == rhs.maxBE
                && maxFrameRetries == rhs.maxFrameRetries
                && maxBackoffRetries == rhs.maxBackoffRetries
                && ackWaitDuration == rhs.ackWaitDuration
                && unitBackoff == rhs.unitBackoff
                && macMode == rhs.macMode
                && txPower == rhs.txPower
                && promiscuousMode == rhs.promiscuousMode;
    }

    virtual bool isValid();

private:
    virtual void doSerialize(ByteVector & buf) const;
    virtual void doUnserialize(ByteVector & buf);
};


} // namespace cometos

#endif /* MACCONFIG_H_ */
