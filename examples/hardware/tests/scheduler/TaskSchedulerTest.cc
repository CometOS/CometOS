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

#include "TaskSchedulerTest.h"
#include "timer1.h"
#include "palLed.h"
#include "Platform.h"
#include "OutputStream.h"

static HappyScheduling hs;
static int irqCounter = 0;
HappyScheduling * HappyScheduling::instance = NULL;


HappyScheduling::HappyScheduling(const char * name) :
        cometos::Module(name),
        counter(0),
        oneSecCounter(0)
{
    ASSERT(instance == NULL);
    instance = this;
}

void HappyScheduling::initialize() {
    for (int i = 0; i < NUM_MESSAGES; i++) {
        cometos::Message * msg = new cometos::Message;
        schedule(msg, &HappyScheduling::fired, intrand(NUM_MESSAGES));
    }
    schedule(&regularMsg, &HappyScheduling::oneSecTask, 1000);
}

void HappyScheduling::oneSecTask(cometos::Message * msg) {
    cometos::getCout() << "Counter=" << oneSecCounter << cometos::endl;
    oneSecCounter++;
    schedule(&regularMsg, &HappyScheduling::oneSecTask, 1000);
}

void HappyScheduling::fired(cometos::Message * msg) {
    counter++;
    if (counter > 1000) {
        palLed_toggle(4);
        counter = 0;
    }
    schedule(msg, &HappyScheduling::fired, intrand(NUM_MESSAGES));
}

void HappyScheduling::irqFired(cometos::Message * msg) {
   if (irqCounter > 1000) {
       palLed_toggle(2);
       irqCounter = 0;
   }
   irqCounter++;
}


void timer1_fire() {
    palLed_toggle(1);
    HappyScheduling::instance->schedule(&(HappyScheduling::instance->irqMsg), &HappyScheduling::irqFired);
    timer1_start(hs.intrand(TIMER_INTVL) + TIMER_INTVL_MIN);
}




int main() {
    cometos::initialize();
    timer1_start(hs.intrand(TIMER_INTVL) + TIMER_INTVL_MIN);
    cometos::run();
}
