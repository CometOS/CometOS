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

/**
 * @author Florian Meier
 */

#ifndef ARBITER_H
#define ARBITER_H

#include "cometos.h"
#include "Callback.h"
#include "OutputStream.h"
#include "cometosError.h"
#include "Task.h"
#include "Event.h"

#define ASSERT_RUNNING(arbiter) ASSERT((arbiter)->isRunning())

namespace cometos {

class Arbiter;

typedef Event ArbiterAction;

class Arbiter : private EventQueue {
public:
    Arbiter();
    void request(ArbiterAction* action);
    cometos_error_t requestImmediately();
    void release();

    bool isEmpty();

    /**
     * Asserts if the arbiter is not currently executing a task.
     *
     * Calling this does not ENSURE that the current context is allowed to execute
     * a guarded method, but at least "SOMEONE" is allowed to. So calling the method
     * without the Arbiter will at least signal an error if no other context is currently
     * running over the same arbiter at the exact same time.
     */
    void assertRunning();

    bool isRunning();

    template<typename... params>
    Callback<void(params...)> generateReleaseCallback() {
        return TOLERANT_CALLBACK(&Arbiter::release, *this, params...);
    }

private:
    Arbiter(Arbiter& xx); // no copy constructor

    Event* user;
    Event* const IMMEDIATE_USER;
};

#if 0
class ActionTask : public BoundedSelfTask<Arbiter,&Arbiter::executeAfterDelay> {
public:
    ActionTask(ArbiterAction* action, Arbiter* arbiter)
    : BoundedSelfTask<Arbiter,&Arbiter::executeAfterDelay>(*arbiter) {
        this->action = action;
    }

    ArbiterAction* getAction() {
        return action;
    }

private:
    ArbiterAction* action;
};
#endif


}

#endif

