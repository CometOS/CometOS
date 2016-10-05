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

#include "palLocalTime.h"
#include <omnetpp.h>
#include <map>
#include "simUtil.h"

using namespace omnetpp;

static std::map<int,double> timeOffsets;

static time_ms_t palLocalTime_get_(cModule* mod) {
    double offset = 0;
    int nodeId=mod->getParentModule()->par("id");

    if (timeOffsets.count(nodeId)) {
        offset = timeOffsets[nodeId];
    }
    return ((time_ms_t) round((simTime().dbl() + offset) * 1000));
}

static void palLocalTime_set_(cModule* mod, time_ms_t time) {
    int nodeId=mod->getParentModule()->par("id");
    timeOffsets[nodeId] = time / 1000.0 - (simTime().dbl());
}

time_ms_t palLocalTime_get() {
    cModule * activeModule = getContextModule();
    if (activeModule != NULL) {
        return palLocalTime_get_(activeModule);
    } else {
        return simTime().dbl();
    }
}

void palLocalTime_set(time_ms_t value) {
    cModule * activeModule = getContextModule();
    if (activeModule != NULL) {
        palLocalTime_set_(activeModule, value);
    } else {
        ASSERT(false);
    }
}

void palLocalTime_init() {
    // nothing to do in simulation
}





