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

#include "mac_interface.h"
#include "atrf_hardware.h"
#include <avr/io.h>

static const uint8_t CCA_MODE_MASK = 0x60;

/** maximum CCA threshold of this transceiver */
static const mac_dbm_t CCA_TH_MAX = -60;

/** minimum CCA threshold of this transceiver */
static const mac_dbm_t CCA_TH_MIN = -90;

mac_result_t mac_setChannel(mac_channel_t channel) {
    // set channel
    if (MAC_MIN_CHANNEL > channel || MAC_MAX_CHANNEL < channel) {
        return MAC_ERROR_FAIL;
    }

    PHY_CC_CCA = ((~CHANNEL_MASK) & PHY_CC_CCA) | channel;
    return MAC_SUCCESS;
}

mac_channel_t mac_getChannel() {
    return CHANNEL_MASK & PHY_CC_CCA;
}

mac_result_t mac_setNetworkId(mac_networkId_t id) {
    if (id == MAC_NWK_BROADCAST) {
        return MAC_ERROR_FAIL;
    }
    PAN_ID_0 = 0xFF & id;
    PAN_ID_1 = 0xFF & (id >> 8);
    return MAC_SUCCESS;
}

mac_networkId_t mac_getNetworkId() {
    return PAN_ID_0 | (((mac_networkId_t) PAN_ID_1) << 8);
}

mac_result_t mac_setNodeId(mac_nodeId_t addr) {
    // set address
    // we allow setting the broadcast address as some kind of
    // uninitialized address
    if (addr == MAC_BROADCAST) {
        return MAC_ERROR_FAIL;
    }
    SHORT_ADDR_0 = 0xff & addr;
    SHORT_ADDR_1 = addr >> 8;
    return MAC_SUCCESS;
}

mac_nodeId_t mac_getNodeId() {
    return (((uint16_t) SHORT_ADDR_1) << 8) | SHORT_ADDR_0;
}


mac_result_t mac_setCCAMode(mac_ccaMode_t mode) {
    if (mode <= MAC_CCA_MODE_ENERGY_AND_CS) {
        PHY_CC_CCA = (PHY_CC_CCA & ~CCA_MODE_MASK) | (mode << CCA_MODE0);
        return MAC_SUCCESS;
    } else {
        return MAC_ERROR_INVALID;
    }
}

mac_ccaMode_t mac_getCCAMode() {
    return (PHY_CC_CCA & CCA_MODE_MASK) >> CCA_MODE0;
}

mac_dbm_t mac_getCCAThreshold() {
    return CCA_TH_MIN + ((CCA_THRES & 0x0F) * 2);
}

mac_result_t mac_setCCAThreshold(mac_dbm_t threshold) {
    if (threshold >= CCA_TH_MIN && threshold <= CCA_TH_MAX) {
        // set threshold accordingly
        uint8_t ccaThresholdValue = (threshold - CCA_TH_MIN) / 2;
        // reset threshold bits
        CCA_THRES &= ~0x0F;
        // set corresponding value
        CCA_THRES |= ccaThresholdValue;
        return MAC_SUCCESS;
    } else {
        return MAC_ERROR_SIZE;
    }
}

