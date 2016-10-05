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

#ifndef TASK_H_
#define TASK_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "types.h"
#include "TaskBase.h"
#include "Callback.h"
#include "Tuple.h"

namespace cometos {
class Module;

class Task : public TaskBase {
	friend class TaskScheduler;
public:
	Task() :
		next(this), expiration(0) {
	}

	virtual ~Task() {}

	virtual void invoke() = 0;

	bool isScheduled() {
		return next != this;
	}

	/**In case of enabled logging a task may define his log level,
	 * this log level can be used to filter log information during execution.*/
	virtual uint8_t getLogLevel() {
		return 0;
	}

private:
    virtual const cometos::Module* getContext() const {
        return NULL;
    }

	Task* next;
	time_ms_t expiration;
};

class SimpleTask: public Task {
public:

	SimpleTask(void(*handler)(void))
	: log_level(0)
	{
		this->handler = handler;
	}
	virtual void invoke() {
		if (handler) {
			handler();
		}
	}
	uint8_t log_level;
	void setLogLevel(uint8_t level) {
		this->log_level = level;
	}
	virtual uint8_t getLogLevel() {
		return this->log_level;
	}

	void (*handler)(void);
};

template<typename C, void(C::*Method)()>
class BoundedTask: public Task {
public:
	BoundedTask(C& instance) :
		instance(instance) {
	}

	virtual void invoke() {
		(instance.*Method)();
	}
private:
	C& instance;
};


template<typename C, void(C::*Method)(Task*)>
class BoundedSelfTask: public Task {
public:
    BoundedSelfTask(C& instance) :
        instance(instance) {
    }

    virtual void invoke() {
        (instance.*Method)(this);
    }
private:
    C& instance;
};


class CallbackTask: public Task {
public:
    CallbackTask(Callback<void()> callback = EMPTY_CALLBACK())
    : callback(callback) {
    }

    void setCallback(Callback<void()> callback) {
        this->callback = callback;
    }

    virtual void invoke() {
        if(callback) {
            callback();
        }
    }

private:
   Callback<void()> callback;
};

}

#endif /* TASK_H_ */
