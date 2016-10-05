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

#ifndef RADIO_ALARM_H
#define RADIO_ALARM_H

#include <tasklet.h>
#include "RadioConfig.h"


/**
 * We use those IDs to workaround the nesc mechanism which dispatches
 * the correct XY_fired() event when the timer is fired.
 *
 * TODO this is neither elegant nor flexible and should be refactored into
 * some class-based solution, possibly using something like a observer
 * interface.
 */
enum {
    SAL_ID = 0,
    CSMAL_ID = 2
};
typedef uint8_t radioAlarmUserId;

void radioAlarm_init();

/**
 * Returns TRUE if the alarm is free and ready to be used. Once the alarm
 * is free, it cannot become nonfree in the same tasklet block. Note,
 * if the alarm is currently set (even if for ourselves) then it is not free.
 */
//tasklet_async command
bool radioAlarm_isFree();

/**
 * Waits till the specified timeout period expires. The alarm must be free.
 */
//tasklet_async command
void radioAlarm_wait(tradio_size timeout, radioAlarmUserId id);

/**
 * Cancels the running alarm. The alarm must be pending.
 */
//tasklet_async command
void radioAlarm_cancel();

/**
 * This event is fired when the specified timeout period expires.
 */
//tasklet_async event
//void radioAlarm_fired();

/**
 * Returns the current time as measured by the radio stack.
 */
//async command
tradio_size radioAlarm_getNow();

void tasklet_radioAlarm_run();

#endif
