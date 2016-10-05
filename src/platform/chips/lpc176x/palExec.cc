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

#include "palExec.h"
#include "palLocalTime.h"
#include "timer_async.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static bool stayAwake;
static time_ms_t lpc176x_counterLastCalled;

extern time_ms_t emRfMain_msTicks;

void palExec_init() {
	stayAwake = false;
	lpc176x_counterLastCalled = emRfMain_msTicks;
	timer_async_init();
}

void palLocalTime_init() {
	timer_async_init();
}

time_ms_t palLocalTime_get() {
	time_ms_t value;
	__asm__ ("cpsid i");
	value = emRfMain_msTicks;
	__asm__ ("cpsie i");
	return value;
}

void palLocalTime_set(time_ms_t value) {
//	__asm__ ("cpsid i");
//	emRfMain_msTicks = value;
//	__asm__ ("cpsie i");
}

time_ms_t palExec_elapsed() {
	time_ms_t value;
	__asm__ ("cpsid i");
	value = emRfMain_msTicks - lpc176x_counterLastCalled;
	lpc176x_counterLastCalled = emRfMain_msTicks;
	__asm__ ("cpsie i");
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
	__asm__ ("cpsid i");
}

void palExec_atomicEnd() {
	__asm__ ("cpsie i");
}

#ifdef __cplusplus
}
#endif
