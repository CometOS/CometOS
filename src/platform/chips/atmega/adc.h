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

#ifndef ADC_H
#define ADC_H

/*INCLUDES *******************************************************************/

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES & MACROS **********************************************************/

/* to signal that the given channel was invalid */
#define ADC_INVALID_CHANNEL    0xFFFF

/* setup adc channels */
enum ADChannels {
  ADC0_CH=0,                            /* ADC0 */
  ADC1_CH,                              /* ADC1 */
  ADC2_CH,                              /* ADC2 */
  ADC_NUM                               /* number of ADC channels*/
};

/* TYPES ********************************************************************/


/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Sets up data directions
 */
void adc_init(void);

/**
 * deaktivates the adc
 */
void adc_disable(void);

/**
 * Reads a value from the given channel
 */
uint16_t adc_read(uint8_t adc_channel);

/**
 * Reads num values from the given channel and builds the average after removing
 * the highest and lowest value. num needs to be at least 3.
 */
uint16_t adc_readAvgWOExtrema(uint8_t adc_channel, uint8_t num);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
