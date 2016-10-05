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

/* INCLUDES ******************************************************************/
#include "adc.h"
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* DEFINES & MACROS **********************************************************/

/* sensor ports and pins */
#define ADC0_PORT               F
#define ADC0_PIN                0
#define ADC1_PORT               F
#define ADC1_PIN                1
#define ADC2_PORT               F
#define ADC2_PIN                2

#define CONCAT(a,b)             a ## b
#define DDR(x)                  CONCAT(DDR, x)
#define PORT(x)                 CONCAT(PORT, x)

/* reference voltage and prescaling */
#define ADC_VREF_SRC             0x01  /* use AVCC Voltage Reference */
#define ADC_VREF_MASK            0xC0
#define ADC_PRESCALE             0x06  /* ADC clock prescaler 2^X, clock must be 330kHz or less for 10 bit resolution */
#define ADC_PRESCALE_MASK        0x07
#define ADC_MUX_MASK             0x1F

/* FUNCTION DEFINITION *******************************************************/

static inline void doConversion() {
    ADCSRA |= (1 << ADEN) | (1 << ADIE);
    set_sleep_mode (SLEEP_MODE_ADC);
    cli();
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
//    do {
//    } while (ADCSRA & (1 << ADSC));

    ADCSRA &= ~((1 << ADEN) | (1 << ADIE));
}


void adc_init(void) {
	// initialize adc pins to input
	DDR(ADC0_PORT) &= ~(1 << ADC0_PIN);
	DDR(ADC1_PORT) &= ~(1 << ADC1_PIN);
	DDR(ADC2_PORT) &= ~(1 << ADC2_PIN);

	// deactivate pull-ups
	PORT(ADC0_PORT) &= ~(1 << ADC0_PIN);
	PORT(ADC1_PORT) &= ~(1 << ADC1_PIN);
	PORT(ADC2_PORT) &= ~(1 << ADC2_PIN);

	PRR0 &= ~(1 << PRADC);  // disable power reduction

	// set ref voltage, right-adjusted and clear MUX bits
	ADMUX = (ADC_VREF_SRC << REFS0);

	/* ADEN   enable ADC
	*  ADSC   start conversion
	*  ADATE  auto triggering
	*  ADIF   clear interrupt flag (cancel pending ADC interrupt)
	*  ADIE   enable ADC interrupt
	*  ADPSx  ADC prescaler (table 27-13)
	*/
	ADCSRA = (1 << ADEN) |
	        (0 << ADSC) |
	        (0 << ADATE) |
	        (1 << ADIF) |
	        (0 << ADIE) |
	        (ADC_PRESCALE << ADPS0);

    //ADCSRA |= (1<<ADSC);                  // Start a conversion
    doConversion();                       // Wait for finish
    /* Need to read ADCW */
    (void) ADCW;

	//sleep_enable();
}

uint16_t adc_read(uint8_t adc_channel) {
    if (adc_channel < ADC_NUM) {

		ADMUX = (ADMUX & ~ADC_MUX_MASK) | (adc_channel & ADC_MUX_MASK);

	    //ADCSRA |= (1<<ADSC);              // Start a conversion
		doConversion();              // Wait for finish

		return ADCW;

	} else {
		return ADC_INVALID_CHANNEL;
	}
}

uint16_t adc_readAvgWOExtrema(uint8_t adc_channel, uint8_t num) {
    if (num < 3) {
        return ADC_INVALID_CHANNEL;
    }
    uint16_t vals[num];
    uint8_t valsMin = 0;
    uint8_t valsMax = 0;
    uint16_t val = 0;
    for (uint8_t i = 0; i < num; i++) {
        vals[i] = adc_read(adc_channel);
        if (vals[i] > vals[valsMax]) {
            valsMax = i;
        }
        if (vals[i] < vals[valsMin]) {
            valsMin = i;
        }
        val += vals[i];
    }
    val = (val - vals[valsMin] - vals[valsMax]) / (num - 2);
    return val;
}

void adc_disable(void) {
	ADCSRA &= ~( (1 << ADEN) | (1 << ADIE) );
	PRR0 |= (1 << PRADC);
}

EMPTY_INTERRUPT(ADC_vect);

