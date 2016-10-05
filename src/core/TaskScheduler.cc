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

#include "TaskScheduler.h"
#include "OutputStream.h"
#include "palExec.h"
#include "cometosAssert.h"
#include "Module.h"
#ifdef PAL_TIME
#include "palLocalTime.h"
#endif
#include "logging.h"

using namespace cometos;

TaskScheduler::TaskScheduler() :
		next(NULL), stopSignal(false), currTask(NULL), currModule(NULL)
#ifdef ENABLE_LOGGING
		        , currLogLevel(LOG_LEVEL_INVALID)
#endif

#ifndef SCHEDULER_DISABLE_MONITORING
				, mon_sleep(0), mon_busy(0), mon_start(0), mon_state(false)
#endif
{
}

#ifndef SCHEDULER_DISABLE_MONITORING
uint8_t TaskScheduler::getUtil() {

#ifdef PAL_TIME
	if (mon_state) {
		mon_busy += palLocalTime_get() - mon_start;
	} else {
		mon_sleep += palLocalTime_get() - mon_start;
	}
	mon_start = palLocalTime_get();
	mon_state = !mon_state;
#endif

	if (mon_busy == 0) {
		mon_busy++;
	}

	uint8_t res = (mon_busy * 100) / (mon_sleep + mon_busy);
	mon_sleep = 0;
	mon_busy = 0;
	return res;
}
#endif

#ifdef ENABLE_LOGGING
uint8_t TaskScheduler::getCurrentLogLevel() {
    return currLogLevel;
}
#endif

/*
 * we do not directly allow to retrieve the task from the scheduler
 * because we can not guarantee that it will not be deleted outside
 * of our control -- this can lead to ugly bugs if someone else,
 * e.g., a logging module, tries to retrieve the context via the
 * (already deleted) task
 */
const Module* TaskScheduler::currentContext() const {
	return currModule;
}

void TaskScheduler::updateExpiration() {
    palExec_atomicBegin();
    // UPDATE TASK LIST
    time_ms_t elapsed = elapsedMS();
    if (elapsed > 0) {
        Task *it = next;
        while (it != NULL) {
            if (it->expiration > elapsed) {
                it->expiration -= elapsed;
            } else {
                it->expiration = 0;
            }
            it = it->next;
        }
    }
    palExec_atomicEnd();
}

time_ms_t TaskScheduler::getTimeUntilNextExecution()
{
    // TRY TO SLEEP
    time_ms_t sleep = (time_ms_t)(-1);
    palExec_atomicBegin();
    if (next) {
        sleep = next->expiration;
    }
    palExec_atomicEnd();

#ifndef SCHEDULER_DISABLE_MONITORING
    if (sleep > 0 && mon_state == true) {
        mon_state = false;
#ifdef PAL_TIME
        mon_busy += palLocalTime_get() - mon_start;
        mon_start = palLocalTime_get();
#endif
    }
#endif

    return sleep;
}

void TaskScheduler::setupNextTask()
{
    // GET AND INVOKE NEXT TASK
    currTask = NULL;
    palExec_atomicBegin();
    if (next != NULL && next->expiration == 0) {
        currTask = next;
        next = next->next;
        currTask->next = currTask; // mark task as not scheduled
    }
    palExec_atomicEnd();
}

void TaskScheduler::wakeup()
{
    palExec_wakeup();
}

time_ms_t TaskScheduler::elapsedMS()
{
    return palExec_elapsed();
}

bool TaskScheduler::run(bool infinite) {
	while (!stopSignal) {

	    time_ms_t sleep = getTimeUntilNextExecution();

		palExec_sleep(sleep);

		setupNextTask();

	    if (currTask) {
#ifdef ENABLE_LOGGING
	        currLogLevel = currTask->getLogLevel();
#endif

#ifndef SCHEDULER_DISABLE_MONITORING
	        if (mon_state == false) {
	            mon_state = true;
#ifdef PAL_TIME
	            mon_sleep += palLocalTime_get() - mon_start;
	            mon_start = palLocalTime_get();
#endif
	        }
#endif
	        currModule = currTask->getContext();
	        currTask->invoke();
	        currModule = NULL;
	        currTask=NULL;
#ifdef ENABLE_LOGGING
	        currLogLevel = LOG_LEVEL_INVALID;
#endif
	    }

	    // updateExpiration after each task execution or sleep period
	    updateExpiration();

		if (infinite == false) {
			return stopSignal;
		}

	}
	return stopSignal;
}

void TaskScheduler::stop() {
	stopSignal = true;
	wakeup();
}

void TaskScheduler::remove_unsafe(Task& task) {
	if (task.isScheduled()) {
		if (next == &task) {
			next = next->next;
			task.next = &task; // mark task as not scheduled
		} else {
			Task* it = next;
			while (it) {
				if (it->next == &task) {
					it->next = task.next;
					task.next = &task; // mark task as not scheduled
					break;
				}
				it = it->next;
			}
		}

	}
}

void TaskScheduler::add_unsafe(Task& task, time_ms_t expiration) {
	if (!task.isScheduled()) {
		task.expiration = expiration;
		if (next == NULL) {
			next = &task;
			task.next = NULL;
		} else if (next->expiration > expiration) {
			task.next = next;
			next = &task;
		} else {
			Task* it = next;
			while (it->next != NULL && it->next->expiration <= expiration) {
				it = it->next;
			}
			task.next = it->next;
			it->next = &task;
		}
	} else {
	    // we consider scheduling a scheduled task a critical failure ---
	    // if the client really wanted to do this, he should have used
	    // replace; otherwise, there is likely a problem in the clients code
	    ASSERT(false);
	}
}

void TaskScheduler::add(Task& task, time_ms_t expiration) {
    // NOTE: we have to update all tasks' expiration, before we add any new
    // task to the scheduler; otherwise, this new task will
    // have its expiration decreased by the duration of the sleep period when
    // updateExpiration is called again after task execution
    updateExpiration();
    palExec_atomicBegin();
	add_unsafe(task, expiration);
	palExec_atomicEnd();
	wakeup();
}

void TaskScheduler::replace(Task& task, time_ms_t expiration) {
    // NOTE: we have to update all tasks' expiration, before we add any new
    // task to the scheduler; otherwise, this new task will
    // have its expiration decreased by the duration of the sleep period when
    // updateExpiration is called again after task execution
    updateExpiration();
	palExec_atomicBegin();
	remove_unsafe(task);
	add_unsafe(task, expiration);
	palExec_atomicEnd();
	wakeup();
}

void TaskScheduler::remove(Task& task) {
	palExec_atomicBegin();
	remove_unsafe(task);
	palExec_atomicEnd();
}

