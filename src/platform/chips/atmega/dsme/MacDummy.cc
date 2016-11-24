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

/*INCLUDES-------------------------------------------------------------------*/

#include "mac_interface.h"
#include "mac_definitions.h"
#include "atrf_hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palExec.h"
#include "palRand.h"
#include "palLocalTime.h"

#include "pinEventOutput.h"

#include "cometos.h"

mac_result_t mac_setBackoffConfig(const mac_backoffCfg_t *cfg) {
    ASSERT(false);
    return 0;
}

void mac_setPromiscuousMode(bool value) {
}

bool mac_getPromiscuousMode() {
    return false;
}

void mac_getAutoAckConfig(mac_ackCfg_t *cfg) {
    ASSERT(false);
}

void mac_getBackoffConfig(mac_backoffCfg_t *cfg) {
    ASSERT(false);
}

mac_result_t mac_setAutoAckConfig(const mac_ackCfg_t *cfg) {
    return true;
}

mac_result_t mac_sendToNetwork(uint8_t const* data, mac_payloadSize_t length,
        mac_nodeId_t dst, mac_networkId_t dstNwk) {
    ASSERT(false);
    return MAC_ERROR_FAIL;
}

mac_result_t mac_send(uint8_t const* data, mac_payloadSize_t length,
        mac_nodeId_t dst) {
    ASSERT(false);
    return MAC_ERROR_FAIL;
}

mac_result_t mac_init(mac_nodeId_t myAddr, mac_networkId_t nwkId,
        mac_channel_t channel, mac_txMode_t mode, mac_ackCfg_t *ackCfg,
        mac_backoffCfg_t *backoffCfg) {
    return MAC_SUCCESS;
}

mac_result_t mac_setReceiveBuffer(uint8_t *buffer) {
    return MAC_SUCCESS;
}
