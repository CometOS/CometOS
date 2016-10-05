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
 * @author Andreas Weigel
 */

#include "LongScheduleModule.h"

namespace cometos {

LongScheduleModule::LongScheduleModule(const char * service_name) :
    RemoteModule(service_name)
{};

LongScheduleModule::~LongScheduleModule()
{}

void LongScheduleModule::initialize() {
    RemoteModule::initialize();

}

void LongScheduleModule::timeout_(LongScheduleMsg * msg)  {
    msg->data.counter--;
    if (msg->data.counter > 0) {
        longScheduleNext(msg);
    } else {
        msg->handler(msg);
    }
}


void LongScheduleModule::longSchedule(LongScheduleMsg * msg, longTimeOffset_t delay) {
    determineLongSchedule(delay, msg->data);
    longScheduleNext(msg);
}

void LongScheduleModule::longCancel(LongScheduleMsg * msg) {
    if (isScheduled(msg)) {
        cancel(msg);
    }
}


void LongScheduleModule::longScheduleNext(LongScheduleMsg * msg) {
    if (isScheduled(msg)) {
        cancel(msg);
    }
    timeOffset_t interval = msg->data.counter > 1 ? MAX_TO_PERIOD : msg->data.rest;
    schedule(msg, &LongScheduleModule::timeout_, interval);
}

void LongScheduleModule::determineLongSchedule(longTimeOffset_t waitFor, LongScheduleMsg::LongScheduleData & data) {
    if (waitFor <= MAX_TO_PERIOD) {
        data.counter = 1;
        data.rest = waitFor;
    } else {
        data.counter = ((waitFor - 1) / MAX_TO_PERIOD) + 1;
        data.rest = waitFor % MAX_TO_PERIOD;
    }
}


} /* namespace cometos */
