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

/**@file Testing TWI/EEPRM in Interrupt Mode
 * @author Andreas Weigel
 */
#include "cometos.h"
#include "logging.h"
#include "Module.h"
#include "palLed.h"
#include "palSpi.h"
#include <util/delay.h>
#include "palPin.h"




using namespace cometos;


#define BUFSIZE 200
DEFINE_INPUT_PIN(spiIrq, PORTD, 2);

typedef uint8_t (*fptr)(uint8_t);

static volatile bool spiDone = false;
static volatile bool spiReady = false;

void error(uint8_t val) {
	while(1) {
		switch(val) {
		case 1:_delay_ms(100); break;
		case 2:_delay_ms(500); break;
		case 3:_delay_ms(1000); break;
		}
		palLed_toggle(4);
	}
}

void sendDone(const uint8_t * tx, uint8_t * rx, uint16_t len, cometos_error_t result) {
	spiDone = true;
}



int main() {
	static uint8_t txbuf[BUFSIZE];
	static uint8_t rxbuf[BUFSIZE];
    cometos::initialize();

    spiIrq_init();
    spiIrq_pullUp();
    EICRA |= (1 << ISC20);
    EIMSK |= (1 << INT2);

    cometos::getCout()<<"StartMaster " << F_CPU <<cometos::endl;

    palLed_init();

    int k = 0;
    cometos_error_t result = palSpi_init(true, 1000000, 3, false);
	if (result != COMETOS_SUCCESS) {
		error(1);
	}
	uint16_t numErrors = 0;


	for (int i = 0; i < BUFSIZE; i++) {
		txbuf[i] = i;
	}
    for (int u=0; u < 10; u++) {
		palLed_on(1);
		while (!spiReady) {
			;
		}
		spiReady = false;
		_delay_ms(200);
		palLed_off(1);


		spiDone = false;
		result = palSpi_transmit(NULL, rxbuf, BUFSIZE, sendDone);
		if (result != COMETOS_SUCCESS) {
			error(2);
		}

		palLed_toggle(2);

		while (!spiDone) {
			;
		}
//		palSpi_enable();
//		result = palSpi_transmitBlocking(txbuf, rxbuf, BUFSIZE);
//		palSpi_disable();
//		if (result != COMETOS_SUCCESS) {
//			if (result == COMETOS_ERROR_BUSY) {
//			    error(2);
//			} else if (result == COMETOS_ERROR_INVALID) {
//			    error(3);
//			}
//		}



		for (uint8_t i = 0; i < BUFSIZE; i++) {
			if (rxbuf[i] != BUFSIZE - i) {
				numErrors++;
			}
		}
		for (int i = 0; i < BUFSIZE; i++) {
			if (i % 16 == 0 && i != 0){
				cometos::getCout() << cometos::endl;
			}
			cometos::getCout() << cometos::hex << (int) rxbuf[i] << " ";
		}
		cometos::getCout() << cometos::endl << cometos::endl;
	}

    cometos::getCout() << cometos::dec << "numErrors= " << numErrors << cometos::endl;

    return 0;
}


ISR (INT2_vect) {
	spiReady = true;
}


#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

