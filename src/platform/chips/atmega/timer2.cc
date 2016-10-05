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

#include "timer2.h"
#include <avr/io.h>
#include <avr/interrupt.h>

void timer2_start(uint8_t interval) {

	uint32_t ticks = F_CPU;

	ticks = (ticks / 1000) * interval; // number of necessary ticks

	// select lowest possible prescaler
	TCNT2 = 0;
	TCCR2A = (1 << WGM21);
	TCCR2B = 0;

	if (ticks < 16384) { // prescaler 64
		TCCR2B |= (1 << CS22);
		ticks = (ticks / 64);
	} else if (ticks < 65536) { // prescaler 256
		TCCR2B |= (1 << CS22) | (1 << CS21);
		ticks /= 256;
	} else if (ticks < 262144) { // prescaler 1024
		TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
		ticks /= 1024;
	} else {
		// NOT REALIZABLE
		return;
	}

	// decrease calculated value by one, 0 to x means x+1 ticks
	OCR2A = (uint8_t) ticks - 1;
	TIMSK2 |= (1 << OCIE2A);
	TIFR2 |= (1 << OCF2A);

}

void timer2_stop() {
	TIMSK2 &= ~(1 << OCIE2A);
	TIFR2 |= (1 << OCF2A);
	TCNT2 = 0;
}


ISR( TIMER2_COMPA_vect )
{
	timer2_fire();
}
