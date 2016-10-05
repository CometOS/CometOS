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

/**@file
 *
 * @author Peter Oppermann (make a platform-independent UART-Test)
 */

#include "cometos.h"
#include "palSerial.h"
#include "palLed.h"
#include "pal.h"
#include "palExec.h"
#include "logging.h"

#ifdef __cplusplus
extern "C" {
#include "cmsis_device.h"
}
#endif

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

#define BUFFER_SIZE 64
#undef SERIAL_BAUDRATE
#define SERIAL_BAUDRATE 500000

static uint8_t rxBuffer[BUFFER_SIZE];
static uint8_t txBuffer[BUFFER_SIZE];
static volatile uint16_t rxPointer, txPointer, rxLength, txLength;

PalSerial* serial;

void rxCallback() {
	uint16_t _rxPointer = rxPointer;
	uint16_t _rxLength = rxLength;

	uint8_t* startPos = rxBuffer + ((_rxPointer + _rxLength) % BUFFER_SIZE) ;
	uint8_t numSigns;
	uint8_t chars;

	if (_rxPointer + _rxLength >= BUFFER_SIZE)
		numSigns = _rxPointer - ((_rxPointer + _rxLength) % BUFFER_SIZE);
	else
		numSigns = BUFFER_SIZE - _rxPointer;

	chars = serial->read((uint8_t*) startPos , numSigns);

	if (chars == numSigns) {
		chars += serial->read((uint8_t*) rxBuffer , BUFFER_SIZE - numSigns);
	}

	//uint16_t bytesWritten = serial->write((uint8_t*) rxBuffer + _rxPointer, chars);

	//	if (bytesWritten != chars)
	//		palLed_toggle(4);

    rxLength += chars;
    if (rxLength > 0) {
        palLed_toggle(1);
    }
    if (rxLength > BUFFER_SIZE)
        rxLength = BUFFER_SIZE;

	palLed_toggle(2);
}

static SimpleTask cbTask(& rxCallback);


int main() {

	rxPointer = 0;
	txPointer = 0;
	rxLength = 0;
	txLength = 0;

	pal_init();

	serial = PalSerial::getInstance(1);
	serial->init(SERIAL_BAUDRATE, &cbTask, 0, 0);
	__enable_irq();

	const char* Greeting = "Hello from the other side \n";
	serial->write((uint8_t*) Greeting, 28);

	while (true) {
		palExec_atomicBegin();
		uint16_t _rxPointer = rxPointer;
		uint16_t _rxLength = rxLength;
		palExec_atomicEnd();

		if (_rxLength > 0) {
			uint16_t length = _rxLength;
			if (_rxPointer + _rxLength > BUFFER_SIZE)
				length = (_rxPointer + _rxLength) - BUFFER_SIZE;

			uint16_t bytesWritten = serial->write((uint8_t*) rxBuffer + _rxPointer, length);

			if (bytesWritten > 0) {

			}

			if (bytesWritten < _rxLength) {
				length = _rxLength - bytesWritten;
				bytesWritten += serial->write((uint8_t*) rxBuffer, length);
			}

			palExec_atomicBegin();
			rxPointer = (rxPointer + bytesWritten) % BUFFER_SIZE;
			rxLength -= bytesWritten;
			palExec_atomicEnd();
		}
	}


	return 0;
}
