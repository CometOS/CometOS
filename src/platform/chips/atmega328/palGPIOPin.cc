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
 * CometOS Platform Abstraction Layer for GPIO interrupts.
 *
 * @author Florian Meier
 */

#include "palGPIOPin.h"
#include "cometos.h"
#include <avr/interrupt.h>

using namespace cometos;

static Callback<void(void)> INTHandler[2];
static uint8_t INTActive = 0;

cometos_error_t GPIOImpl::setupInterrupt(IOPort port, uint8_t pin, IGPIOPin::Edge type, Callback<void(void)> callback) {
    if(port == IOPort::D && (pin == 2 || pin == 3)) {
        // INT
        uint8_t intNum = pin-2;
        bool active = true;
        uint8_t isctype = 0;

        palExec_atomicBegin();

        // deactivate interrupt first
        EIMSK &= ~(1 << intNum); 

        // parse type
        switch(type) {
            case IGPIOPin::Edge::RISING:
                isctype = 0x03;
                break;
            case IGPIOPin::Edge::FALLING:
                isctype = 0x02;
                break;
            case IGPIOPin::Edge::ANY_EDGE:
                isctype = 0x01;
                break;
            case IGPIOPin::Edge::LOW_LEVEL:
                isctype = 0x00;
                break;
            case IGPIOPin::Edge::NONE:
                active = false;
                break;
            default:
                return COMETOS_ERROR_INVALID;
        }

        // write control register
        uint8_t pos = intNum*2;
        EICRA &= ~(0x3 << pos);
        EICRA |= (isctype << pos);

        INTHandler[intNum] = callback;
        INTActive |= (1 << (intNum));

        // activate interrupt
        if(active) {
            EIMSK |= (1 << intNum); 
        }

        palExec_atomicEnd();

        return COMETOS_SUCCESS;
    }
    else {
        // PCINT
        return COMETOS_ERROR_INVALID;
    }
}

void GPIOImpl::interruptActive(IOPort port, uint8_t pin, bool enable) {
    if(port == IOPort::D && (pin == 2 || pin == 3)) {
        uint8_t intNum = pin-2;
        if(enable) {
            INTActive |= (1 << (intNum));
        }
        else {
            INTActive &= ~(1 << (intNum));
        }
    }
    else {
        ASSERT(false);
    }
/*
    else if(port == IOPort::B) {
        if(enable) {
            PCINTActive |= (1 << (pin));
        }
        else {
            PCINTActive &= ~(1 << (pin));
        }
    }
*/
}

inline static void genINTIRQ(uint8_t intNum) {
    if((INTActive & (1 << intNum)) && INTHandler[intNum]) {
        INTHandler[intNum]();
    }
}

ISR(INT0_vect)
{
    genINTIRQ(0);
}

ISR(INT1_vect)
{
    genINTIRQ(1);
}

