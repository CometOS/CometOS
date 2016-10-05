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
 * @author Florian Meier
 */

#include "palRS485.h"
#include "palPin.h"
#include "palSerial.h"
#include "cometos.h"
#include <util/delay.h>
#include <stddef.h>

namespace cometos {

/*MACROS---------------------------------------------------------------------*/

#define UART 1

DEFINE_OUTPUT_PIN_INV(read_enable,PORTD,5);
DEFINE_OUTPUT_PIN(write_enable,PORTD,6);


/*VARIABLES------------------------------------------------------------------*/



/*FUNCTION DEFINITION--------------------------------------------------------*/

static Task* txCallback;

// actually call this callback for txEnd
// then call the original txEndCallback after resetting the pins
static void palRS485_txCallback() {
	write_enable_off();
	_delay_us(5); // wait for tPHZ,tPLZ (Driver disable time)
	read_enable_on();
	_delay_us(9); // wait for tPZH,tPZL (Receiver enable time)

	if(txCallback != NULL) {
		txCallback->invoke();
	}
}
SimpleTask rs485_txCallback(palRS485_txCallback);

PalSerial* serial = NULL;

void palRS485_init(uint32_t baudrate, Task* rxStartCallback,
		Task* txFifoEmptyCallback, Task* txReadyCallback) {
	txCallback = txReadyCallback;

	read_enable_init();
	write_enable_init();

	write_enable_off();
	read_enable_on();

	serial = PalSerial::getInstance(UART);
	serial->init(baudrate,rxStartCallback,txFifoEmptyCallback,&rs485_txCallback);
}

void palRS485_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback) {
	serial->set_9bit_mode(enable, multiProcessorMode, rxByteCallback);
}

uint8_t palRS485_write(const uint8_t* data, uint8_t length, bool flagFirst) {
	// The receiver is first switched off, then the driver is switched on
	// This takes longer than the other way around, but is safer in terms of reading garbage
	read_enable_off();
	write_enable_on();

	_delay_us(14); // wait for tPZH,tPZL (Driver enable time, 12us) 
                   // plus wait additionally for receiver to switch from transmit to receive

	return serial->write(data,length,flagFirst);
}

uint8_t palRS485_read(uint8_t* data, uint8_t length) {
	return serial->read(data,length);
}
}
