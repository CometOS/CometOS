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

#include "LowpanAdaptionLayerStructures.h"
#include "Vector.h"

namespace cometos_v6 {

const uint8_t LowpanConfigConstants::DEFAULT_MIN_DELAY_NOMINATOR;
const uint8_t LowpanConfigConstants::DEFAULT_MIN_DELAY_DENOMINATOR;
const uint8_t LowpanConfigConstants::DEFAULT_QUEUE_SWITCH_AFTER;
const uint16_t LowpanAdaptionLayerStats::DURATION_FACTOR;
const uint16_t LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;

uint32_t avgDurationMs(const LowpanAdaptionLayerStats& lals) {
        return lals.avgDuration >> LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;
}

bool LowpanConfig::operator==(const LowpanConfig & rhs) {
    return this->macRetryControlMode == rhs.macRetryControlMode
        && this->delayMode == rhs.delayMode
        && this->queueType == rhs.queueType
        && this->minDelayNominator == rhs.minDelayNominator
        && this->minDelayDenominator == rhs.minDelayDenominator
        && this->enableLFFR == rhs.enableLFFR
        && this->numReassemblyHandlers == rhs.numReassemblyHandlers
        && this->bufferSize == rhs.bufferSize
        && this->numBufferHandlers == rhs.numBufferHandlers
        && this->numDirectDatagramHandlers == rhs.numDirectDatagramHandlers
        && this->enableDirectFwd == rhs.enableDirectFwd
        && this->numIndicationMsgs == rhs.numIndicationMsgs
        && this->congestionControlType == rhs.congestionControlType
        && this->timeoutMs == rhs.timeoutMs
        && this->queueSwitchAfter == rhs.queueSwitchAfter
        && this->pushBackStaleObjects == rhs.pushBackStaleObjects
        && this->queueSize == rhs.queueSize
        && this->useRateTimerForQueueSwitch == rhs.useRateTimerForQueueSwitch;
}

bool LowpanConfig::isValid() {
    return macRetryControlMode >= LOWPAN_MACCONTROL_DEFAULT
            && macRetryControlMode <= LOWPAN_MACCONTROL_MAX_VALUE
            && delayMode >= DM_NONE
            && delayMode <= DM_MAX_VALUE
            && ((queueType == LowpanConfigConstants::QT_FIFO && congestionControlType==LowpanConfigConstants::CCT_NONE)
                    || (queueType == LowpanConfigConstants::QT_DG_ORDERED && congestionControlType == LowpanConfigConstants::CCT_DG_ORDERED))
            && numReassemblyHandlers < 50
            && bufferSize <= 10240
            && numBufferHandlers <= 160
            && numDirectDatagramHandlers <= 40
            && numIndicationMsgs <= numReassemblyHandlers
	    && queueSize <= numBufferHandlers;
}

void LowpanConfig::doSerialize(cometos::ByteVector& buf) const {
    serialize(buf, this->delayMode);
    serialize(buf, this->macRetryControlMode);
    serialize(buf, this->queueType);
    serialize(buf, this->minDelayNominator);
    serialize(buf, this->minDelayDenominator);
    serialize(buf, this->enableLFFR);
    serialize(buf, this->numReassemblyHandlers);
    serialize(buf, this->bufferSize);
    serialize(buf, this->numBufferHandlers);
    serialize(buf, this->numDirectDatagramHandlers);
    serialize(buf, this->enableDirectFwd);
    serialize(buf, this->numIndicationMsgs);
    serialize(buf, this->congestionControlType);
    serialize(buf, this->timeoutMs);
    serialize(buf, this->queueSwitchAfter);
    serialize(buf, this->pushBackStaleObjects);
    serialize(buf, this->queueSize);
    serialize(buf, this->useRateTimerForQueueSwitch);
}

void LowpanConfig::doUnserialize(cometos::ByteVector& buf) {
    unserialize(buf, this->useRateTimerForQueueSwitch);
    unserialize(buf, this->queueSize);
    unserialize(buf, this->pushBackStaleObjects);
    unserialize(buf, this->queueSwitchAfter);
    unserialize(buf, this->timeoutMs);
    unserialize(buf, this->congestionControlType);
    unserialize(buf, this->numIndicationMsgs);
    unserialize(buf, this->enableDirectFwd);
    unserialize(buf, this->numDirectDatagramHandlers);
    unserialize(buf, this->numBufferHandlers);
    unserialize(buf, this->bufferSize);
    unserialize(buf, this->numReassemblyHandlers);
    unserialize(buf, this->enableLFFR);
    unserialize(buf, this->minDelayDenominator);
    unserialize(buf, this->minDelayNominator);
    unserialize(buf, this->queueType);
    unserialize(buf, this->macRetryControlMode);
    unserialize(buf, this->delayMode);
}



}
