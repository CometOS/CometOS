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
#include "s25fl_blocking.h"
#include <util/delay.h>
#include "timer1.h"
#include <avr/interrupt.h>

using namespace cometos;

static volatile uint16_t ofs = 0;

void error(uint8_t val) {
    while(1) {
        switch(val) {
        case 1:_delay_ms(100); break;
        case 2:_delay_ms(500); break;
        case 3:_delay_ms(1000); break;
        }
        palLed_toggle(1);
    }
}

#define BUFSIZE 256

void timer1_fire() {
    ofs++;
}

int main() {
    uint8_t rx[BUFSIZE];

    uint8_t tx[BUFSIZE] = {0x31, 0x40, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20};

    uint8_t status = 0xAB;

    cometos::getCout() << "Starting blocking flash test" << cometos::endl;

    // init leds and flash mem
    palLed_init();

    if (s25fl_init() != COMETOS_SUCCESS) {
        error(1);
    }

    _delay_ms(1000);
    palLed_on(1);
    cometos_error_t result;

    result = s25fl_sectorErase(0x000000);
    if (result != COMETOS_SUCCESS){
        error(3);
    }
    while (0x01 & status) {
        result = s25fl_readStatus(&status);
    }

    _delay_ms(3000);
    result = s25fl_pageProgram(0x0, tx, 12);
    if (result != COMETOS_SUCCESS) {
        error(1);
    }

    while (0x01 & status) {
        result = s25fl_readStatus(&status);
    }

    timer1_start(50000);
    sei();

    uint16_t counter = timer1_get();
    timer1_stop();
    uint32_t passed = (ofs * 50000 + counter) * 4;


    //while(true) {
        result = s25fl_readStatus(&status);
        cometos::getCout() << cometos::hex << "schubidu:" << (int) status << cometos::endl;
        _delay_ms(1000);
        if (result != COMETOS_SUCCESS){
            error(1);
        }
    //}



        result = s25fl_read(0, rx, BUFSIZE);
        if (result != COMETOS_SUCCESS) {
            error(2);
        }



        _delay_ms(3000);
        cometos::getCout() << (int) result << "|" << (int) status << "|" << passed << cometos::endl;
        for (int i = 0; i < BUFSIZE; i++) {
            if (i % 16 == 0 && i != 0){
                cometos::getCout() << cometos::endl;
            }
            cometos::getCout() << cometos::hex << (int) rx[i] << " ";
        }
        cometos::getCout() << cometos::endl << cometos::endl << cometos::dec;
//    }
        while(true);
    return 0;
}



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

