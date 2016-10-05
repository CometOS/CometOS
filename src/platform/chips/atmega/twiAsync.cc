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

/*INCLUDES-------------------------------------------------------------------*/
#include "twiAsync.h"
#include <avr/interrupt.h>

/*VARIABLES------------------------------------------------------------------*/
static bool initialized = false;

/*FUNCTION DEFINITION--------------------------------------------------------*/
void twiAsync_init() {
	if (initialized) {
		return;
	}
	initialized = true;

	TWSR = 0;
	TWBR = (F_CPU / 100000UL - 16) / 2;
}

void twiAsync_start() {
	// wait for ongoing stop operations
	while (TWCR & (1<<TWSTO)) {}

	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
}

void twiAsync_stop() {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);
}

void twiAsync_receive(bool ack) {
	if (ack) {
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
	} else {
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
	}
}

void twiAsync_send(uint8_t data) {
	// set data
	TWDR = data;
	// start transmission of data
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
}

ISR( TWI_vect) {
	twiAsync_doneCallback(TWDR, TW_STATUS);
}
