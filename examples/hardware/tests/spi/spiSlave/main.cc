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
 * @author Stefan Unterschuetz
 */
#include "cometos.h"
#include "logging.h"
#include "Module.h"
#include "palLed.h"
#include "palSpi.h"
#include "palPin.h"

using namespace cometos;

DEFINE_OUTPUT_PIN(spiReady, PORTD, 2);
#define BUFSIZE 200


static volatile bool done = false;

void receiveDone(const uint8_t * tx, uint8_t * rx, uint16_t len, cometos_error_t result) {
	done = true;
}



int main() {
    cometos::initialize();

    cometos::getCout()<<"StartSlave"<<cometos::endl;
    if (COMETOS_SUCCESS != palSpi_init(false, 0, 3, false)) {
    	palLed_on(4);
    }


    _delay_ms(1000);
    uint8_t rxbuf[BUFSIZE];
    uint8_t txbuf[BUFSIZE];

    for (int i = 0; i < BUFSIZE; i++) {
    	txbuf[i] = BUFSIZE - i;
    }

    bool first = true;
    uint8_t numErrors = 0;
    for (int u= 0; u < 10; u++) {
    	done = false;
    	if (first) {
			spiReady_init();
			first = false;
		} else {
			spiReady_toggle();
		}
//		if (COMETOS_SUCCESS != palSpi_transmitBlocking(txbuf, rxbuf, BUFSIZE)) {
//			palLed_on(4);
//		}
//		done = true;

		if (COMETOS_SUCCESS != palSpi_transmit(txbuf, NULL, BUFSIZE, receiveDone)) {
			palLed_on(4);
		}

		palLed_toggle(2);
		while (!done) {
			;
		}


		for (int k=0; k<BUFSIZE; k++) {
			if (rxbuf[k] != k) {
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
    cometos::getCout() << cometos::dec << "numErrors=" << (int) numErrors << cometos::endl;

    return 0;
}



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

