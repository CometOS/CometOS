#include <tasklet.h>
#include "palExec.h"
#include "logging.h"

#include "RFA1DriverLayer.h"
#include "RadioAlarm.h"
#include "MessageBufferLayer.h"

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


#ifdef TASKLET_IS_TASK

#include "TaskScheduler.h"

    task void tasklet()
    {
        signal Tasklet.run();
    }

    inline async command void Tasklet.schedule()
    {
        post tasklet();
    }

    inline command void Tasklet.suspend()
    {
    }

    inline command void Tasklet.resume()
    {
    }

#else

void tasklet_run() {
    tasklet_radio_run();
}

/**
 * The lower 7 bits contain the number of suspends plus one if the run
 * event is currently beeing executed. The highest bit is set if the run
 * event needs to be called again when the suspend count goes down to zero.
 */
static uint8_t tasklet_state;

void doit() {
    for(;;) {
        tasklet_run();

        palExec_atomicBegin();
        {
            if( tasklet_state == 1 )
            {
                tasklet_state = 0;
                palExec_atomicEnd();
                return;
            }

            ASSERT( tasklet_state == 0x81 );
            tasklet_state = 1;
        }
        palExec_atomicEnd();
    }
}

void tasklet_suspend() {
    palExec_atomicBegin();
    ++tasklet_state;
    palExec_atomicEnd();
}

void tasklet_resume()
{
    palExec_atomicBegin();
    {
        if( --tasklet_state != 0x80 ) {
            palExec_atomicEnd();
            return;
        }

        tasklet_state = 1;
    }
    palExec_atomicEnd();
    doit();
}

//async
void tasklet_schedule() {
    palExec_atomicBegin();
    {
        if( tasklet_state != 0 )
        {
            tasklet_state |= 0x80;
            palExec_atomicEnd();
            return;
        }

        tasklet_state = 1;
    }
    palExec_atomicEnd();
    doit();
}

#endif
