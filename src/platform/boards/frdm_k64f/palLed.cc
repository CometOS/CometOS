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
 * CometOS Platform Abstraction Layer for LED Control.
 *
 * @author Stefan Unterschuetz
 */

#include "palLed.h"
#include "palPin.h"

DEFINE_OUTPUT_PIN_INV(ledRed,PORTB,22);
DEFINE_OUTPUT_PIN_INV(ledGreen,PORTE,26);
DEFINE_OUTPUT_PIN_INV(ledBlue,PORTB,21);

void palLed_init() {
	ledRed_init();
	ledGreen_init();
	ledBlue_init();
}

void palLed_toggle(uint8_t mask) {
	if (mask & 1)
		ledRed_toggle();
	if (mask & 2)
		ledGreen_toggle();
	if (mask & 4)
		ledBlue_toggle();
}

void palLed_on(uint8_t mask) {
	if (mask & 1)
		ledRed_on();
	if (mask & 2)
		ledGreen_on();
	if (mask & 4)
		ledBlue_on();
}

void palLed_off(uint8_t mask) {
	if (mask & 1)
		ledRed_off();
	if (mask & 2)
		ledGreen_off();
	if (mask & 4)
		ledBlue_off();
}

