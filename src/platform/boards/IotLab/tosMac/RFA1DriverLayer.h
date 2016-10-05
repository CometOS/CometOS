/*
 * Copyright (c) 2007, Vanderbilt University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Miklos Maroti
 * Author: Andras Biro
 * Author: Andreas Weigel (modifications for CometOS)
 */

#ifndef __RFA1DRIVERLAYER_H__
#define __RFA1DRIVERLAYER_H__

#include "tosMsgReplace.h"
#include "mac_definitions.h"
#include "cometosError.h"
#include "mac_interface.h"

typedef uint8_t mcu_power_t;

mac_result_t RFA1Driver_init(mac_nodeId_t myAddr, mac_networkId_t nwkId,
        mac_channel_t channel, mac_txMode_t mode, mac_ackCfg_t *ackCfg,
        mac_backoffCfg_t *backoffCfg);


/* ------------ COMMANDS ---------------------------------------------- */
mac_result_t radioSend_send(message_t* msg);

mac_result_t radioCCA_request();

/* ------------ EVENTS ------------------------------------------------ */
void radioSend_sendDone(message_t* msg, mac_result_t result);

message_t* radioReceive_receive(message_t* msg);

void radioSend_ready();

void radioCCA_done(mac_result_t result);

void tasklet_radio_run();

#endif//__RFA1DRIVERLAYER_H__
