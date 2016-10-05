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

#ifndef TASKSCHEDULER_H_
#define TASKSCHEDULER_H_

#include "Task.h"

#define LOG_LEVEL_INVALID 0xFF

namespace cometos {

class TaskScheduler;
TaskScheduler &getScheduler();

/**
 *
 */
class TaskScheduler {
public:

	/**Initializes the task scheduler in idle mode.
	 * Using RC-clock is recommended (lower energy consumption, because of faster wakeup).
	 * RC-clock may not work well with uart communication.
	 */
	TaskScheduler();

	/**Runs scheduler. The scheduler is fair, thus no starvation is
	 * possible.
	 *
	 *
	 * Note that tasks are removed from task list, when calling
	 */
	bool run(bool infinite);

	/**Stop the scheduler. Will be executed in synchronous cntext. All
	 * current runnable tasks (timer=0) are executed before scheduler is stopped.
	 * */
	void stop();


#ifndef SCHEDULER_DISABLE_MONITORING
	/**Gets utilization of Scheduler, resets statistics after call
	 * This call is not thread-safe (you are only allowed to call it
	 * from a task)
	 * */
	uint8_t getUtil();
#endif

	/**Adds task to scheduler. A FIFO is used for tasks with same expiration
	 * time. A task is called in synchronous context. This is thread-safe.
	 * A pointer to the passed instance of task is internally stored. Hence,
	 * the task should not be deleted by the main app.
	 * If task is already scheduled, this task will do nothing.
	 *
	 * @param expiration 	timer until task is executed in milli seconds
	 *
	 * Note that asynchronous tasks should be short
	 * to avoid long interrupt service routines
	 */
	void add(Task& task, time_ms_t expiration = 0);

	/**
	 * Adds task to scheduler. If the task is already scheduled in scheduler
	 * instance it is removed.
	 */
	void replace(Task& task, time_ms_t expiration = 0);

	/**
	 * @return returns pointer to the module currently being executed
	 *         if not in context of any module, NULL is returned
	 */
	const Module* currentContext() const;
	uint8_t getCurrentLogLevel();

	/**
	 * Removes Message, but doesn't delete memory!
	 *
	 * @return the number of removed occurences
	 */
	void remove(Task& task);

protected:
	time_ms_t getTimeUntilNextExecution();
    void setupNextTask();
    virtual void wakeup();
    virtual time_ms_t elapsedMS();

	void updateExpiration();
	void remove_unsafe(Task& task);
	virtual void add_unsafe(Task& task, time_ms_t expiration = 0);

	Task* next;
	volatile bool stopSignal;
	Task* currTask;
	const Module* currModule;
	uint8_t currLogLevel;

#ifndef SCHEDULER_DISABLE_MONITORING
	time_ms_t mon_sleep;
	time_ms_t mon_busy;
	time_ms_t mon_start;
	bool mon_state;
#endif
};

}

#endif /* TASKSCHEDULER_H_ */
