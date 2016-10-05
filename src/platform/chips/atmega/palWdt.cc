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

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "palPers.h"
#include "palWdt.h"
#include <stddef.h>
#include "palExecUtils.h"
#include "palLed.h"

static const uint8_t palWdt_numSteps = 10;
static bool palWdt_running = false;

static uint8_t* stack;
static const uint8_t flag = 1;

enum {
    WDTO_0 = 15,
    WDTO_1 = 30,
    WDTO_2 = 60,
    WDTO_3 = 120,
    WDTO_4 = 250,
    WDTO_5 = 500,
    WDTO_6 = 1000,
    WDTO_7 = 2000,
    WDTO_8 = 4000,
    WDTO_9 = 8000
};

static const uint16_t palWdt_values[palWdt_numSteps] = {WDTO_0, WDTO_1, WDTO_2, WDTO_3, WDTO_4, WDTO_5, WDTO_6, WDTO_7, WDTO_8, WDTO_9};
static palWdt_callback palWdt_cb;

static uint16_t palWdt_timeout_back;
static palWdt_callback palWdt_callback_back;

uint16_t palWdt_enable(uint16_t timeout, palWdt_callback cb) {
    wdt_reset();

    palWdt_timeout_back = timeout;
    palWdt_callback_back = cb;

    uint8_t i = 0;
    if (timeout <= palWdt_values[0] || timeout >= palWdt_values[palWdt_numSteps-1]) {
        if (timeout >= palWdt_values[0]) {
            i = palWdt_numSteps - 1;
        }
    } else {
        while(palWdt_values[i] <= timeout && i < palWdt_numSteps) {
            i++;
        }
        i--;
    }

    wdt_enable(i);
    palWdt_running = true;

    // activate watchdog interrupt!
    WDTCSR|=(1<<WDIE);

    if (cb != NULL) {
        palWdt_cb = cb;
    }
    return palWdt_values[i];
}

void palWdt_pause() {
    wdt_disable();
}

void palWdt_resume() {
    palWdt_enable(palWdt_timeout_back, palWdt_callback_back);
}

bool palWdt_isRunning() {
    return palWdt_running;
}

/**
 * Resets the watchdog timer. Needs to be called regularly to prevent watchdog
 * from firing.
 */
void palWdt_reset() {
    wdt_reset();
}

// TODO check if we are not risking to produce garbage due to calling
//      functions within a NAKED ISR, currently, this does not seem to be the
//      case
ISR( WDT_vect, ISR_NAKED) {
    // get address of program counter
    stack= (reinterpret_cast<uint8_t*>(SP))+1;
    palPers_write(PM_WATCHDOG_RESET_FLAG, &flag, 1);
    palExec_writePc(stack);
    wdt_reset();
    if (palWdt_cb != NULL) {
        palWdt_cb();
    }
    // next watchdog interrupt will reset node
    while (1) {
        palLed_toggle(4);
        _delay_ms(250);
    }
}
