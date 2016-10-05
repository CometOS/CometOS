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

#include "Arbiter.h"
#include "palExec.h"
#include "TaskScheduler.h"

using namespace cometos;

Arbiter::Arbiter()
: user(nullptr),
  IMMEDIATE_USER((Event* const)(-1))
{
}

void Arbiter::request(ArbiterAction* action)
{
    palExec_atomicBegin();
    push(action);

    // Schedule if no current user
    // There is at least one (the new one)
    if(!user) {
        user = getFront();
        scheduleFrontEvent();
    }

    palExec_atomicEnd();
}

cometos_error_t Arbiter::requestImmediately() {
    palExec_atomicBegin();
    if(user != nullptr) {
        return COMETOS_ERROR_BUSY;
    }
    else {
       user = IMMEDIATE_USER;
    }
    palExec_atomicEnd();

    return COMETOS_SUCCESS;
}

void Arbiter::assertRunning()
{

    ASSERT(user != nullptr);
}

bool Arbiter::isRunning()
{
    return (user != nullptr);
}

bool Arbiter::isEmpty()
{
    palExec_atomicBegin();
    Event* tmp = getFront();
    palExec_atomicEnd();
    return (tmp == nullptr);
}

void Arbiter::release()
{
    assertRunning();

    palExec_atomicBegin();

    // Schedule if there is another user
    user = getFront();
    if(user != nullptr) {
        scheduleFrontEvent();
    }

    palExec_atomicEnd();
}

