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

#include <util/delay.h>
#include <avr/interrupt.h>

#include "logging.h"
#include "palFirmware.h"
#include <string.h>
#include "palLed.h"
#include "palPers.h"
#include "palExecUtils.h"
#include "OutputStream.h"

#include "cometosAssert.h"

void resetTheNode() {
    cli();

#if 0
    // print the stack
    for(uint16_t adr = SP; adr < SP+300; adr++) {
    	uint8_t val = *((uint8_t*)adr);
    	if(val <= 0xF) {
    		cometos::getSerialCout() << "0";
    	}
    	cometos::getSerialCout() << cometos::hex << (uint16_t)val << " ";
    }
#endif

    for(uint32_t i = 0; i < 50; i++) {
       palLed_toggle(2);
       _delay_ms(1000);
    }

    palExec_reset();
}

#ifdef ASSERT_SHORT
    #ifdef SERIAL_ASSERT
    void doAssert(uint16_t line, uint16_t fileId) {
        cometos::getCout() << "A:" << cometos::dec << fileId << ":" << line << cometos::endl;
        resetTheNode();
    }
    #else
    void doAssert(uint16_t line, uint16_t fileId) {
        palLed_on(4);
        palExec_writeAssertInfoShort(line, fileId);
        resetTheNode();
    }
    #endif
#else

#ifdef ASSERT_LONG
    #ifdef SERIAL_ASSERT
    void doAssert(uint16_t line, const char* filename) {
        cli();
        cometos::getCout() << "A:" << cometos::dec << line << ":" << filename  << cometos::endl;
        resetTheNode();
    }
    #else
    void doAssert(uint16_t line, const char* filename) {
        palExec_writeAssertInfoLong(line, filename);
        resetTheNode();
    }
    #endif
#else

    #ifdef SERIAL_ASSERT
    void doAssert(uint32_t id) {
        cli();
        cometos::getCout() << "A:" << cometos::hex << "0x" << id << cometos::endl;
        resetTheNode();
    }
    #else
    void doAssert(uint32_t id) {
        resetTheNode();
    }
    #endif

#endif
#endif

