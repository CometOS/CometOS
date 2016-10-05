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

extern "C" {
#include "device/fsl_device_registers.h"
}

#include "timer2.h"

#define PIT_PRESCALER 1

void timer2_start(uint8_t interval) {
	// Enable clock gate for PIT
	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

	// Enable IRQ
	NVIC_EnableIRQ(PIT0_IRQn);

	// Enable PIT module
	PIT_MCR &= ~PIT_MCR_MDIS_MASK;

	// Calculate counter value
	uint32_t ticks = F_CPU;
	ticks = (ticks / 1000) * interval;
	PIT_LDVAL0 = ticks >> PIT_PRESCALER;

	// Enable interrupt and timer
	PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;
}

void timer2_stop() {
	// Disable interrupt and timer
	PIT_TCTRL0 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
}

extern "C" void PIT0_IRQHandler(void)
{
	// reset IRQ
	PIT_TFLG0 |= PIT_TFLG_TIF_MASK;

	// fire callback
	timer2_fire();
}
