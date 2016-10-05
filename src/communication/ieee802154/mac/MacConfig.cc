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

#include "MacConfig.h"

namespace cometos {

void MacConfig::doSerialize(ByteVector & buf) const {
    serialize(buf, this->ccaMode);
    serialize(buf, this->ccaThreshold);
    serialize(buf, this->nwkId);
    serialize(buf, this->channel);
    serialize(buf, this->minBE);
    serialize(buf, this->maxBE);
    serialize(buf, this->maxFrameRetries);
    serialize(buf, this->maxBackoffRetries);
    serialize(buf, this->ackWaitDuration);
    serialize(buf, this->unitBackoff);
    serialize(buf, this->macMode);
    serialize(buf, this->txPower);
    serialize(buf, this->promiscuousMode);
}

void MacConfig::doUnserialize(ByteVector & buf) {
    unserialize(buf, this->promiscuousMode);
    unserialize(buf, this->txPower);
    unserialize(buf, this->macMode);
    unserialize(buf, this->unitBackoff);
    unserialize(buf, this->ackWaitDuration);
    unserialize(buf, this->maxBackoffRetries);
    unserialize(buf, this->maxFrameRetries);
    unserialize(buf, this->maxBE);
    unserialize(buf, this->minBE);
    unserialize(buf, this->channel);
    unserialize(buf, this->nwkId);
    unserialize(buf, this->ccaThreshold);
    unserialize(buf, this->ccaMode);
}

bool MacConfig::isValid() {
    // TODO define constants for border values
    return txPower >= mac_getMinTxPowerLvl()
            && txPower <= mac_getMaxTxPowerLvl()
            && macMode < 8
            && unitBackoff == 320
            && ackWaitDuration >= 544
            && ackWaitDuration <= 864
            && maxBackoffRetries <= 5
            && maxFrameRetries <= 15
            && maxBE <= 8
            && minBE >= 0
            && minBE <= maxBE
            && channel >= MAC_MIN_CHANNEL
            && channel <= MAC_MAX_CHANNEL
            && ccaThreshold <= -60
            && ccaThreshold >= -90
            && ccaMode >= 0
            && ccaMode <= 3;
}




} // namespace cometos
