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

/**
 * Minimum power level the transceiver used by this MAL supports; the
 * minimum power level SHALL correspond to the minimum actual transmission power
 */
#define MAC_MIN_TX_POWER_LEVEL 0 	// -17 dBm

/**
 * Maximum power level the transceiver used by this MAL supports; the
 * maximum power level SHALL correspond to the maximum actual transmission power
 */
#define MAC_MAX_TX_POWER_LEVEL 15	//   3 dBm

uint8_t mac_getMinTxPowerLvl() {
    return MAC_MIN_TX_POWER_LEVEL;
}

uint8_t mac_getMaxTxPowerLvl() {
    return MAC_MAX_TX_POWER_LEVEL;
}

mac_result_t mac_setTxPower(uint8_t pwrLevel) {
    if (pwrLevel > MAC_MAX_TX_POWER_LEVEL) {
        return MAC_ERROR_FAIL;
    }
    PHY_TX_PWR = 0xF & (MAC_MAX_TX_POWER_LEVEL - pwrLevel);
    return MAC_SUCCESS;
}

uint8_t mac_getTxPower() {
    return MAC_MAX_TX_POWER_LEVEL - (PHY_TX_PWR & 0xF);
}

mac_result_t mac_setRxLnaState(bool enable) {
    return MAC_SUCCESS;
}

mac_result_t mac_on() {
    //TODO
    return MAC_SUCCESS;
}

mac_result_t mac_sleep() {
    //TODO
    return MAC_SUCCESS;
}

mac_result_t mac_enableAutomaticDiversity() {
    return MAC_SUCCESS;
}

mac_result_t mac_disableAutomaticDiversity(uint8_t selected_antenna) {
    return MAC_SUCCESS;
}
