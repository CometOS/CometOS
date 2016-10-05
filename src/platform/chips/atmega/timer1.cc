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

#include "timer1.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/** used as 62.5 kHz radio alarm timer */

void timer1_init() {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12);

    // prescaler 64
//    TCCR1B |= (1 << CS11) | (1 << CS10);
    // prescaler 256
    TCCR1B |= (1 << CS12);
}

void timer1_start(uint16_t interval) {
	TCNT1 = 0;

	OCR1A = interval;

	TIFR1 |= (1 << OCF1A);
	TIMSK1 |= (1 << OCIE1A);
}

void timer1_stop() {
	TIMSK1 &= ~(1 << OCIE1A);
	TIFR1 |= (1 << OCF1A);
	TCNT1 = 0;
}

uint16_t timer1_get() {
    return TCNT1;
}

ISR( TIMER1_COMPA_vect )
{
	timer1_fire();
}
