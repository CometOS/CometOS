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

#ifndef ASYNC_ACTION_H
#define ASYNC_ACTION_H

#include "Arbiter.h"
#include "cometosError.h"
#include "palExec.h"
#include "Tuple.h"
#include "LoadableTask.h"

namespace cometos {

template<typename... Params>
class AsyncAction : public ArbiterAction {
private:
    void granted()
    {
        task.invoke();

        palExec_atomicBegin();
        busy = false;
        palExec_atomicEnd();
    }

    friend class Arbiter;

    BoundedTask<AsyncAction, &AsyncAction::granted> grantedTask;

public:
    AsyncAction()
    : ArbiterAction(), grantedTask(*this), busy(false)
    {
        Callback<void()> cb;
        cb = (Task*)&grantedTask;
        setCallback(cb);
    }

    void setAction(Callback<void(Params...)> action)
    {
        task.setCallback(action);
    }

    void setArbiter(Arbiter* arbiter)
    {
        this->arbiter = arbiter;
    }

    cometos_error_t execute(Params... params)
    {
        palExec_atomicBegin(); 
        if(busy) {
            palExec_atomicEnd();
            return COMETOS_ERROR_BUSY;
        }
        busy = true;
        palExec_atomicEnd(); 

        task.load(params...);

        ASSERT(arbiter != NULL);
        arbiter->request(this);

        return COMETOS_PENDING;
    }

    // TODO since this is executed blocking anyway, there is not need for this in AsyncAction!
    cometos_error_t executeImmediately(Params... params)
    {
        palExec_atomicBegin(); 
        if(busy) {
            return COMETOS_ERROR_BUSY;
        } 
        busy = true;
        palExec_atomicEnd(); 

        this->callback = EMPTY_CALLBACK();

        task.load(params...);

        ASSERT(arbiter != NULL);
        cometos_error_t result = arbiter->requestImmediately(this);
        if(result == COMETOS_SUCCESS) {
            task.invoke();
        }

        return result;
    }

protected:
    Arbiter* arbiter;
    LoadableTask<Params...> task;
    bool busy;
};

template<typename... Params>
class DelayableAsyncAction : public AsyncAction<Params...> {
private:
    void executeAfterDelay() {
        this->arbiter->request(this);
    }

public:
    DelayableAsyncAction()
     : delayTask(MakeCallback(&DelayableAsyncAction<Params...>::executeAfterDelay).template Create<&DelayableAsyncAction<Params...>::executeAfterDelay>(this)) {
    }

    cometos_error_t executeDelayed(Params... params, timeOffset_t delay)
    {
        palExec_atomicBegin(); 
        if(this->busy) {
            return COMETOS_ERROR_BUSY;
        } 
        this->busy = true;
        palExec_atomicEnd(); 

        this->task.load(params...);

        ASSERT(this->arbiter != NULL);
        getScheduler().add(delayTask, delay);

        return COMETOS_PENDING;
    }

private:
    CallbackTask delayTask; // for delayed execution
};


}

#endif
