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

//tasklet_norace
static uint8_t csmaState;
enum
{
    STATE_READY = 0,
    STATE_TX_PENDING_FIRST = 1,
    STATE_TX_PENDING_SECOND = 2,
    STATE_TX_SENDING = 3,

    STATE_BARRIER = 0x80,
};

static void calcNextRandom();

//tasklet_norace
static message_t *trclTxMsg;
//tasklet_norace
static uint16_t txBarrier;

SimpleTask tCalcNextRandom(calcNextRandom);

mac_backoffCfg_t csmaCfg;

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


MacType tosUtil_getMacType() {
    return MT_ORIGINAL_TOS;
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

static uint16_t getBackoff(uint16_t maxBackoff)
{
    uint16_t a;

    palExec_atomicBegin();
    {
        a = nextRandom;
        nextRandom += 273;
    }
    palExec_atomicEnd();
    //post -- random calculation so complex to outsource it to a task?
    cometos::getScheduler().add(tCalcNextRandom);

    return (a % maxBackoff) + tosUtil_getMinBackoff(); // Config.getMinimumBackoff();
}

//tasklet_async command
mac_result_t csmaSend_send(message_t* msg)
{
    if( csmaState != STATE_READY || ! radioAlarm_isFree() )
        return MAC_ERROR_BUSY;

    trclTxMsg = msg;
    csmaState = STATE_TX_PENDING_FIRST;
    radioAlarm_wait(getBackoff(tosUtil_getInitialBackoff()), CSMAL_ID); //Config.getInitialBackoff(msg)

    return MAC_SUCCESS;
}

//tasklet_async event
void csmaRadioAlarm_fired()
{
    mac_result_t error;
    int16_t delay;

    ASSERT( csmaState != STATE_READY );

    delay = (int16_t)txBarrier - radioAlarm_getNow();

    if( csmaState == STATE_BARRIER )
    {
        csmaState = STATE_READY;

        csmaSend_ready();
        return;
    }
    else if( (csmaState & STATE_BARRIER) && delay > 0 ){
        error = MAC_ERROR_BUSY;
    } else {
        error = salSend_send(trclTxMsg);
    }

    if( error != MAC_SUCCESS ) {
        if( (csmaState & ~STATE_BARRIER) == STATE_TX_PENDING_FIRST ) {
            csmaState = (csmaState & STATE_BARRIER) | STATE_TX_PENDING_SECOND;
            radioAlarm_wait(getBackoff(tosUtil_getCongestionBackoff()), CSMAL_ID); //Config.getCongestionBackoff(trclTxMsg)
        } else {
            if( (csmaState & STATE_BARRIER) && delay > 0 ) {
                csmaState = STATE_BARRIER;
                radioAlarm_wait(delay, CSMAL_ID);
            }
            else {
                csmaState = STATE_READY;
            }

            csmaSend_sendDone(trclTxMsg, error);
        }
    } else {
        csmaState = STATE_TX_SENDING;
    }
}

//tasklet_async event
void salSend_sendDone(message_t* msg, mac_result_t error)
{
    ASSERT( csmaState == STATE_TX_SENDING );
    ASSERT( msg == trclTxMsg);
    trclTxMsg = NULL;
    csmaState = STATE_READY;
    csmaSend_sendDone(msg, error);
}

////tasklet_async event
//bool salReceive_header(message_t* msg)
//{
//    return csmaReceive_header(msg);
//}

//tasklet_async event
message_t* salReceive_receive(message_t* msg)
{
    int16_t delay;

    txBarrier = tosUtil_getTransmitBarrier(msg); //Config.getTransmitBarrier(msg);
    delay = txBarrier - radioAlarm_getNow();

    if( delay > 0 )
    {
        if( csmaState == STATE_READY )
        {
            // disregard the barrier for now, this needs a better solution
            if( radioAlarm_isFree() )
            {
                radioAlarm_wait(delay, CSMAL_ID);
                csmaState = STATE_BARRIER;
            }
        }
        else
            csmaState |= STATE_BARRIER;
    }

    return csmaReceive_receive(msg);
}
