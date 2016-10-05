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
 * @author Stefan Unterschuetz
 * @author Andreas Weigel
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "palExec.h"
#include "palLocalTime.h"
#include "timer2.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <util/atomic.h>

#include "logging.h"
#include "cometos.h"

static bool stayAwake;
static time_ms_t counter = 0;
static time_ms_t time;

//static int putchar_dummy(char c, FILE *stream) {return 0;}
//static FILE dummystdout= FDEV_SETUP_STREAM( putchar_dummy, NULL, _FDEV_SETUP_WRITE );

void timer2_fire() {
	counter++;
	time++;
}

void palExec_init() {
    //stdout = &dummystdout;

	timer2_start(1);
	stayAwake = false;
	counter = 0;
}

void palLocalTime_init() {
	time = 0;
}

time_ms_t palLocalTime_get() {
	time_ms_t value;
	palExec_atomicBegin();
	value = time;
	palExec_atomicEnd();
	return value;
}

//void palLocalTime_delay(time_ms_t busyTime) {
//    _delay_ms(busyTime);
//}

time_ms_t palExec_elapsed() {
	time_ms_t value;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        value = counter;
        counter = 0;
	}
	return value;
}

void palExec_sleep(time_ms_t ms) {
	if (stayAwake || ms == 0) {
		stayAwake = false;
	} else {

		// currently we do nothing here
	}
}

void palExec_wakeup() {
	stayAwake = true;
}

static uint8_t inAtomic = 0;
static bool alreadyAtomic = false;

void palExec_atomicBegin() {
    uint8_t tmp = SREG;
	cli();

    ASSERT(inAtomic < UINT8_MAX);

	if(inAtomic == 0 && !(tmp & (1 << 7))) {
	    // We are already in an atomic section (e.g. ISR)
	    alreadyAtomic = true;
	}

    inAtomic++;
}

void palExec_atomicEnd() {
    ASSERT(inAtomic >= 1);
    inAtomic--;

    if(inAtomic == 0) {
        if(alreadyAtomic) {
            alreadyAtomic = false;
        }
        else {
            sei();
        }
    }
}
