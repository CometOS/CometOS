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

#include "palLed.h"
#include "palPin.h"
#include "cmsis_device.h"

DEFINE_OUTPUT_PIN_INV(greenLed, 1, 5)
DEFINE_OUTPUT_PIN_INV(redLed, 3, 2)
DEFINE_OUTPUT_PIN_INV(orangeLed, 2, 10)




void palLed_init(){
	RCC->APB2ENR |= (1 << RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN);

	greenLed_init();
	redLed_init();
	orangeLed_init();

}

void palLed_on(uint8_t mask){
	if(mask & 0x1)
		greenLed_on();

	if(mask & 0x2)
		redLed_on();

	if(mask & 0x4)
		orangeLed_on();
}

void palLed_off(uint8_t mask){
	if(mask & 0x1)
		greenLed_off();

	if(mask & 0x2)
		redLed_off();

	if(mask & 0x4)
		orangeLed_off();
}

void palLed_toggle(uint8_t mask){
	if(mask & 0x1)
		greenLed_toggle();

	if(mask & 0x2)
		redLed_toggle();

	if(mask & 0x4)
		orangeLed_toggle();
}
