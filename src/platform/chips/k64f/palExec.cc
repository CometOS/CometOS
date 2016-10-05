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
 * @author Florian Meier (for ARM)
 */

extern "C" {
#include "device/fsl_device_registers.h"
}

#include "palExec.h"
#include "palLocalTime.h"
#include "timer2.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


#ifdef FNET
#if FNET
extern "C" {
#include "fnet.h"
#include "fnet_timer_prv.h"
}
#endif
#endif


static bool stayAwake;
static time_ms_t counter;
static time_ms_t time;

#ifdef FNET
#if FNET
static time_ms_t fnet_period_ms;
static time_ms_t fnet_period_cnt;
#endif
#endif

// TODO
//static int putchar_dummy(char c, FILE *stream) {return 0;}
//static FILE dummystdout= FDEV_SETUP_STREAM( putchar_dummy, NULL, _FDEV_SETUP_WRITE );

void timer2_fire() {
	counter++;
	time++;

#ifdef FNET
#if FNET
	fnet_period_cnt++;
	if(fnet_period_cnt >= fnet_period_ms) {
		fnet_timer_ticks_inc();
		fnet_period_cnt = 0;
	}
#endif
#endif
}

#ifdef FNET
#if FNET
int fnet_os_timer_init(unsigned int period_ms) 
{
	fnet_period_ms = period_ms;
	fnet_period_cnt = 0;

	return FNET_OK;
}

void fnet_os_timer_release() {}
#endif
#endif

void palExec_init() {
    // TODO stdout = &dummystdout;

	timer2_start(1);
	stayAwake = false;
	counter = 0;
}

void palLocalTime_init() {
	time = 0;
}

void palExec_reset() {
    NVIC_SystemReset();
}

time_ms_t palLocalTime_get() {
	time_ms_t value;
	__disable_irq();
	value = time;
	__enable_irq();
	return value;
}

//void palLocalTime_delay(time_ms_t busyTime) {
    //_delay_ms(busyTime);
//}

time_ms_t palExec_elapsed() {
	time_ms_t value;
	__disable_irq();
	value = counter;
	counter = 0;
	__enable_irq();
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

void palExec_atomicBegin() {
	__disable_irq();
}

void palExec_atomicEnd() {
    __enable_irq();
}


