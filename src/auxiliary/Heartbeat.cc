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

#include "Heartbeat.h"
#include "palLed.h"
#include "palPers.h"
#include "palId.h"
#include "OutputStream.h"
#include "palWdt.h"
#include "NetworkTime.h"

#define STACK_PROTECTION_VALUE 0xdeadbeef

namespace cometos {

volatile uint32_t checkVal __attribute__((section (".stackprot")));

Heartbeat::Heartbeat(const char* name) :
        cometos::Module(name) {
    checkVal = STACK_PROTECTION_VALUE;

    // We put the initialization of the WDT into the constructor because this
    // should happen before any of the pals or modules calls its initialize
    // function. Failing to initialize the watchdog this early, we may run into
    // an assert which then causes the system to enter an infinite loop.
    // This may still happen if another module ctor is called before this one
    // (which it is difficult to prevent) and reaches an ASSERT.
#ifdef PAL_WDT
    palWdt_reset();
    palWdt_enable(4000, NULL);
#endif
}

void Heartbeat::initialize() {
    cometos::Module::initialize();
    scheduleLed();
}

void Heartbeat::toggleLed(cometos::Message * msg) {
	time_ms_t mt = NetworkTime::get();

#ifdef PAL_WDT
    palWdt_reset();
#endif
    if ((mt / INTERVAL) % 2 == 1) {
        palLed_on(1);
    } else {
        palLed_off(1);
    }

    ASSERT(checkVal == STACK_PROTECTION_VALUE);

    scheduleLed();
}

void Heartbeat::scheduleLed() {
	time_ms_t mt = NetworkTime::get();
    timeOffset_t delay = INTERVAL - (mt % INTERVAL);
    if (delay == 0) {
        delay = INTERVAL;
    }
    schedule(&aliveMsg, &Heartbeat::toggleLed, delay);
}


} // namespace cometos
