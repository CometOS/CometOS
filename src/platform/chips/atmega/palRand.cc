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
 * @author Andreas Weigel
 */

#include "palId.h"
#include <avr/io.h>
#include <util/delay.h>
#include "palLed.h"
#include "palRand.h"

#define RX_ON                           6
#define TRX_OFF                         8
#define PLL_ON                          9

#define CMD_FORCE_PLL_ON                4
#define CMD_RX_ON                       6
#define CMD_PLL_ON                      9
#define CMD_FORCE_TRX_OFF               3

#define TRANSCEIVER_STATE (TRX_STATUS & 0x1F)

static unsigned int cometos_rnd = 0;

unsigned int palRand_get() {
    return cometos_rnd;
}

void palRand_init() {
    TRX_STATE = CMD_RX_ON;

    while (TRANSCEIVER_STATE != RX_ON)
       ;

    unsigned int rnd = 0;
    _delay_ms(1);
    for (uint8_t i=0; i < sizeof(rnd) * 8 / 2; i++) {
       _delay_us(1);
       rnd = (rnd << 2) | ((PHY_RSSI >> RND_VALUE0) & 0x03);
    }
    TRX_STATE = CMD_FORCE_TRX_OFF;

    cometos_rnd = rnd;

    while (TRANSCEIVER_STATE != TRX_OFF)
       ;
}


