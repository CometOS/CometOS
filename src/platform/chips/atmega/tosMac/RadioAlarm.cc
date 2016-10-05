/*
 * Copyright (c) 2013, Eric B_ Decker
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
 * Author: Eric B. Decker <cire831@gmail.com>
 * Author: Andreas Weigel (modifications for CometOS)
 *
 * The rfxlink stack is primarily intended to be run using tasklets with
 * interrupts on.  Tasklets provide for reasonable mutual exclusion (like
 * tasks with run to completion) while having the response latency from
 * running at interrupt level.
 *
 * RadioAlarm want to have interrupts reenabled.  This is done in Alarm.fired
 * which is the handler that initially gets notified when a timer interrupt
 * has fired.
 */

//#include <tasklet.h>
#include "RadioAlarm.h"
#include "RadioConfig.h"
#include "cometos.h"
#include "palExec.h"
#include "timer1.h"

#include "RFA1DriverLayer.h"
#include "SoftwareAckLayer.h"
#include "CsmaLayer.h"

//norace
static uint8_t state;
enum
{
    STATE_READY = 0,
    STATE_WAIT = 1,
    STATE_FIRED = 2,
};

static radioAlarmUserId currUser;

void radioAlarm_init() {
    timer1_init();
}

//async event
void timer1_fire()
{
    timer1_stop();
    sei(); //< re-enable interrupts, as timer1 uses default ISR
    palExec_atomicBegin();
    {
        if( state == STATE_WAIT ) {
            state = STATE_FIRED;
        }
    }
    palExec_atomicEnd();

//    radioAlarm_fired();
    tasklet_schedule();
}

//async command
tradio_size radioAlarm_getNow()
{
    return timer1_get();
}

//tasklet_async event
void tasklet_radioAlarm_run()
{
    if( state == STATE_FIRED )
    {
        state = STATE_READY;
        switch (currUser){
        case SAL_ID: {
            salRadioAlarm_fired();
            break;
        }
        // radio driver does not use the actual timer
//        case RDL_ID: {
//            radioRadioAlarm_fired();
//            break;
//        }
        case CSMAL_ID: {
            csmaRadioAlarm_fired();
            break;
        }
        default: {
            ASSERT(false);
        }
        }
    }
}

//default tasklet_async event void radioAlarm_fired[uint8_t id]()
//{
//}
//tasklet_async command
bool radioAlarm_isFree()
{
    return state == STATE_READY;
}

//tasklet_async command
void radioAlarm_wait(tradio_size timeout, radioAlarmUserId id)
{
    currUser = id;
    ASSERT( state == STATE_READY );

    state = STATE_WAIT;
    timer1_start(timeout);
}

//tasklet_async command
void radioAlarm_cancel()
{
    ASSERT( state != STATE_READY );

    timer1_stop();
    state = STATE_READY;
}
