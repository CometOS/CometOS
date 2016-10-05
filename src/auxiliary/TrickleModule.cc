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

#include "TrickleModule.h"
#include "logging.h"

namespace cometos {

TrickleModule::TrickleModule(const char * service_name, uint16_t iMin, uint8_t iMax, uint8_t k) :
        LongScheduleModule(service_name),
        iUnit(iMin),
        iMax(iMax),
        k(k),
        iCurr(0),
        c(0),
        txTimer(createCallback(&TrickleModule::txTimeout_)),
        intervalTimer(createCallback(&TrickleModule::intervalTimeout_))
    {}



 TrickleModule::~TrickleModule() {}

 void TrickleModule::setTrickleModule(uint16_t iMin, uint8_t iMax, uint8_t k){
     bool active = isActive();
     stop_();
     this->iUnit = iMin;
     this->iMax = iMax;
     this->k = k;

     if (active) {
         start_();
     }
}

 void TrickleModule::initialize() {
    RemoteModule::initialize();
}

 void TrickleModule::finish() {
     cancelTimers();
 }

void TrickleModule::transmissionReceived_(bool isConsistent) {
    if (!isConsistent) {
        if (iCurr != iMin) {
            reset_();
        } else {
            // do nothing if we are already at minimum interval
        }
    } else {
        c++;
    }
}

bool TrickleModule::isActive() {
    return isScheduled(&intervalTimer);
}

bool TrickleModule::start_() {
    if (!isActive()) {

        // set starting interval to some value in [0,iMax]
        iCurr = intrand(iMax+1);
        startInterval_();
    #ifdef OMNETPP
        last = omnetpp::simTime();
    #endif
    }
    return true;
}

bool TrickleModule::stop_() {
    if (isActive()) {
        cancelTimers();
        return true;
    } else {
        return false;
    }
}

void TrickleModule::reset_() {
    LOG_INFO("");
    iCurr = iMin;
    cancelTimers();
    startInterval_();
}


void TrickleModule::txTimeout_(LongScheduleMsg * msg) {
    if (c < k) {
#ifdef OMNETPP
       sendVector.record(omnetpp::simTime() - last);
       last = omnetpp::simTime();
#endif
       transmit();
   } else {
       LOG_INFO("Suppress tx");
   }
}

void TrickleModule::intervalTimeout_(LongScheduleMsg * msg) {
    iCurr = iCurr < iMax ? iCurr+1 : iMax;
    startInterval_();
}


void TrickleModule::startInterval_() {
    c = 0;
    uint32_t currI = ((uint32_t) iUnit) << iCurr;
    uint32_t iHalf = currI >> 1;

    // determine time of possible transmission t and schedule timer
    uint32_t t = iHalf + intrand(iHalf);
    LOG_INFO("Schedule tx in " << t);
    longSchedule(&txTimer, t);

    // schedule interval end timer
    longSchedule(&intervalTimer, currI);
}

inline void TrickleModule::cancelTimers() {
    cancel(&txTimer);
    cancel(&intervalTimer);
}

} // namespace cometos
