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
 * Author: Andreas Weigel (modifications for CometOS)
 */

#include "tasklet.h"
#include "logging.h"
#include "CsmaLayer.h"
#include "RadioAlarm.h"
#include "cometos.h"
#include "palExec.h"
#include "Task.h"
#include "tosUtil.h"
#include "SoftwareAckLayer.h"
#include "RadioConfig.h"
#include "OutputStream.h"

//tasklet_norace

enum eCsmaState : uint8_t
{
    STATE_READY = 0,
    STATE_TX_PENDING = 1,
    STATE_TX_SENDING = 2,
    STATE_TX_BLOCKED = 3
};
static eCsmaState csmaState;

static void calcNextRandom();
void csmaRadioAlarm_fired();

//tasklet_norace
static message_t * csmaTxMsg;

mac_backoffCfg_t csmaCfg;

static uint8_t nb; //< NB from 802.15.4_2006 (number of backoffs)
//static uint8_t be; //< BE from 802.15.4_2006 (current backoff exponent)

cometos::SimpleTask tCalcNextRandom(calcNextRandom);
cometos::SimpleTask tFireRadioTimer(csmaRadioAlarm_fired);

mac_result_t mac_setBackoffConfig(const mac_backoffCfg_t *cfg) {
    csmaCfg.maxBE = cfg->maxBE;
    csmaCfg.minBE = cfg->minBE;
    csmaCfg.maxBackoffRetries = cfg->maxBackoffRetries;
    csmaCfg.unitBackoff = cfg->unitBackoff;
    return MAC_SUCCESS;
}

void mac_getBackoffConfig(mac_backoffCfg_t * cfg) {
    cfg->maxBE = csmaCfg.maxBE;
    cfg->minBE = csmaCfg.minBE;
    cfg->maxBackoffRetries = csmaCfg.maxBackoffRetries;
    cfg->unitBackoff = csmaCfg.unitBackoff;
};

inline void incrementNumLocalAndAccumulatedBackoffs(message_t * msg) {
    nb++;
    msg->txInfo.numBackoffs++;
}

MacType tosUtil_getMacType() {
    return MT_802154_TOS;
}

//tasklet_async event
void salSend_ready()
{
    if( csmaState == STATE_READY && radioAlarm_isFree() ) {
        csmaSend_ready();
    }
}

static uint16_t nextRandom;
//task
static void calcNextRandom()
{
    uint16_t x = 0xFFFF;
    uint16_t a = intrand(x); //Random.rand16();
    palExec_atomicBegin();
    nextRandom = a;
    palExec_atomicEnd();
}

static uint16_t getNextBackoff()
{
    uint8_t backoffExp = csmaTxMsg->backoffOffset + csmaCfg.minBE + nb;
    backoffExp = backoffExp <= csmaCfg.maxBE ? backoffExp : csmaCfg.maxBE;
    uint8_t numUnitBackoffs = intrand(1 << backoffExp);

    uint16_t val = ((uint16_t) numUnitBackoffs) * (MAC_DEFAULT_UNIT_BACKOFF / RADIO_ALARM_MICROSEC_PER_TICK);
//    cometos::getCout() << "mbe=" << (int) csmaCfg.minBE << "|be=" << (int) backoffExp << "|bo=" << val << "|nub=" << (int)numUnitBackoffs << cometos::endl;
    return val;
}

inline void backoff() {
    uint16_t delay = getNextBackoff();
    if (delay > 0) {
        radioAlarm_wait(delay, CSMAL_ID); //Config.getInitialBackoff(msg)
    } else {
        // instead of immediately calling csmaRadioAlarm_fired() -- which can
        // cause a loop over all backoffs if they are always 0 and break
        // the MessageBufferLayer -- we schedule a task, which should be safe
        // though we will lose quite a bit time
        cometos::getScheduler().add(tFireRadioTimer);
        //csmaRadioAlarm_fired();
    }
}

//tasklet_async command
mac_result_t csmaSend_send(message_t* msg)
{
    // initialize "local" number of backoffs to zero
    nb = 0;
    if( csmaState != STATE_READY || ! radioAlarm_isFree() )
        return MAC_ERROR_BUSY;

    csmaTxMsg = msg;
    csmaState = STATE_TX_PENDING;
    backoff();

    // with each backoff, we increase the number of backoffs attached to the
    // message (considering ALL possible retries) and the local count nb
    incrementNumLocalAndAccumulatedBackoffs(csmaTxMsg);

    return MAC_SUCCESS;
}

void radioBusy(mac_result_t error) {
    if (nb <= csmaCfg.maxBackoffRetries) {
        backoff();
        incrementNumLocalAndAccumulatedBackoffs(csmaTxMsg);
    } else {
        message_t * originalMsg = csmaTxMsg;
        csmaState = STATE_READY;
        csmaTxMsg = NULL;
        csmaSend_sendDone(originalMsg, error);
    }
}

//tasklet_async event
void csmaRadioAlarm_fired()
{
    mac_result_t error;

    ASSERT( csmaState != STATE_READY );

    error = salSend_send(csmaTxMsg);

    if (error != MAC_SUCCESS) {

        // radio busy due to other obligations
        radioBusy(error);
    } else {
        csmaState = STATE_TX_SENDING;
    }
}

//tasklet_async event
void salSend_sendDone(message_t* msg, mac_result_t error)
{
    ASSERT( csmaState == STATE_TX_SENDING );
    ASSERT( msg == csmaTxMsg);

    if (error == MAC_ERROR_BUSY) {
        radioBusy(error);
    } else {
        csmaState = STATE_READY;
        csmaTxMsg = NULL;
        csmaSend_sendDone(msg, error);
    }
}


//tasklet_async event
message_t* salReceive_receive(message_t* msg)
{
    return csmaReceive_receive(msg);
}
