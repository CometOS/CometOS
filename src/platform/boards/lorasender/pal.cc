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
 * @author Stefan Unterschuetz
 */

#include <avr/interrupt.h>
#include "pal.h"
#include "palExec.h"
#include "palExecUtils.h"
#include <string.h>

#include "palRand.h"
#include <stdlib.h>

#ifdef PAL_ID
#include "palId.h"
#endif

#ifdef PAL_LED
#include "palLed.h"
#endif

#ifdef PAL_PERS
#include "palPers.h"
#endif

#if defined PAL_FIRMWARE || defined PAL_FIRMWARE_ASYNC
#include "palFirmware.h"
#endif



#ifdef __cplusplus
extern "C" {
#endif
void __cxa_pure_virtual() {
}
#ifdef __cplusplus
}
#endif



#include <avr/io.h>
#include "OutputStream.h"

#ifndef SERIAL_PRINTF_BAUDRATE
#ifdef SERIAL_PRINTF
#define SERIAL_PRINTF_BAUDRATE SERIAL_PRINTF
#else
#define SERIAL_PRINTF_BAUDRATE 57600
#endif
#endif

static void serial_init() {
    // set baud rate
    uint16_t baud_rate_reg = (((F_CPU) + 4UL * (SERIAL_PRINTF_BAUDRATE)) / (8UL
            * (SERIAL_PRINTF_BAUDRATE)) - 1UL);
    UBRR0H = (baud_rate_reg >> 8) & 0xFF;
    UBRR0L = (uint8_t) baud_rate_reg;
    // Faster async mode (UART clock divider = 8, instead of 16)
    UCSR0A = (1 << U2X0);
    // Data Length is 8 bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    // Transmitter is enabled.
    UCSR0B = (1 << TXEN0);
}

static void serial_putchar(char c) {
	// Wait for empty transmit buffer
	while (!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = c;
}

namespace cometos {

static OutputStream cout(serial_putchar);

OutputStream  &getSerialCout() {
    static bool init=false;
    if (init==false) {
        init=true;
        serial_init();
    }
    return cout;
}

#ifdef SERIAL_PRINTF
OutputStream  &getCout() {
    return getSerialCout();
}
#endif

}





void pal_init() {
#ifdef PAL_LED
    palLed_init();
#endif
#ifdef PAL_ID
    palId_init();
#endif
#ifdef PAL_RAND
    palRand_init();
    srand(palRand_get());
#elif defined PAL_ID
    srand(palId_id());
#else
    srand(0);
#endif
#ifdef PAL_PERS
	palPers_init();
#endif
#if defined PAL_FIRMWARE || defined PAL_FIRMWARE_ASYNC
	if (PAL_FIRMWARE_SUCCESS != palFirmware_init()) {
	    palLed_on(4);
	}
#endif
	palExec_init();
	sei();
}

#define DUMMYISR(name) ISR(name##_vect) {\
        cometos::getSerialCout() << __LINE__ << cometos::endl;\
        palExec_badisr();\
}

//DUMMYISR(INT0)
//DUMMYISR(INT1)
//DUMMYISR(PCINT0)
DUMMYISR(PCINT1)
DUMMYISR(PCINT2)
//DUMMYISR(WDT)
//DUMMYISR(TIMER2_COMPA)
//DUMMYISR(TIMER2_COMPB)
DUMMYISR(TIMER2_OVF)
DUMMYISR(TIMER1_COMPA)
DUMMYISR(TIMER1_COMPB)
DUMMYISR(TIMER1_OVF)
DUMMYISR(TIMER1_CAPT)
DUMMYISR(TIMER0_COMPA)
DUMMYISR(TIMER0_COMPB)
DUMMYISR(TIMER0_OVF)
//DUMMYISR(SPI_STC)
//DUMMYISR(USART_RX)
//DUMMYISR(USART_UDRE)
//DUMMYISR(USART_TX)
DUMMYISR(ADC)
DUMMYISR(EE_READY)
DUMMYISR(ANALOG_COMP)
//DUMMYISR(TWI)
DUMMYISR(SPM_READY)

