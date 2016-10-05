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

#ifndef LONGSCHEDULE_H_
#define LONGSCHEDULE_H_

#include "RemoteModule.h"

namespace cometos {

class LongScheduleMsg : public cometos::Message {
    friend class LongScheduleModule;

public:
    LongScheduleMsg(const TypedDelegate<LongScheduleMsg> & handler) :
        handler(handler)
    {}

private:
    struct LongScheduleData {
        typedef uint16_t counter_t;
        counter_t counter;
        timeOffset_t rest;
    };

    TypedDelegate<LongScheduleMsg> handler;
    LongScheduleData data;
};


/**
 * Utility module to be used as a base class for modules which need to schedule
 * longer periods of time. This module offers to schedule a function call up to
 * what an uint32_t can hold in ms the future (a bit more than 49 days, which
 * should be enough for most applciations).
 *
 * TODO currently, actually only max. 65535 * 65535 ms are supported, it
 * should be possible to change this
 */
class LongScheduleModule : public RemoteModule {
public:
    typedef uint32_t longTimeOffset_t;

    LongScheduleModule(const char * service_name);

    virtual ~LongScheduleModule();

    virtual void initialize();

    /**
     * Schedule LongScheduleMsg's handler to be executed after offset ms.
     * @param msg     object containing a TypedDelegate to the method to execute
     * @param offset  time in milliseconds when the method should be called
     */
    void longSchedule(LongScheduleMsg * msg, longTimeOffset_t offset);

    void longCancel(LongScheduleMsg * msg);

private:
    static const timeOffset_t MAX_TO_PERIOD = -1;

    void timeout_(LongScheduleMsg * msg);

    void longScheduleNext(LongScheduleMsg * msg);

    void determineLongSchedule(longTimeOffset_t waitFor, LongScheduleMsg::LongScheduleData & data);
};


} /* namespace cometos */
#endif /* LONGSCHEDULE_H_ */
