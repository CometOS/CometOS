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

static Callback<void(void)> INTHandler[8];
static uint8_t INTActive = 0;
static Callback<void(void)> PCINTHandler[8];
static uint8_t PCINTActive = 0;

cometos_error_t GPIOImpl::setupInterrupt(IOPort port, uint8_t pin, IGPIOPin::Edge type, Callback<void(void)> callback) {
    if(port == IOPort::E || port == IOPort::D) {
        // INT
        bool active = true;
        uint8_t isctype = 0;

        palExec_atomicBegin();

        // deactivate interrupt first
        EIMSK &= ~(1 << pin); 

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
        uint8_t pos = (pin%4)*2;
        if(pin <= 3) {
            EICRA &= ~(0x3 << pos);
            EICRA |= (isctype << pos);
        }
        else {
            EICRB &= ~(0x3 << pos);
            EICRB |= (isctype << pos);
        }

        INTHandler[pin] = callback;
        INTActive |= (1 << (pin));

        // activate interrupt
        if(active) {
            EIMSK |= (1 << pin); 
        }

        palExec_atomicEnd();

        return COMETOS_SUCCESS;
    }
    else if(port == IOPort::B) {
        // PCINT
        //PCINTHandler[pin] = callback;
        //PCINTActive |= (1 << (pin));
        return COMETOS_ERROR_INVALID;
    }
    else {
        return COMETOS_ERROR_INVALID;
    }
}

void GPIOImpl::interruptActive(IOPort port, uint8_t pin, bool enable) {
    if(port == IOPort::E || port == IOPort::D) {
        if(enable) {
            INTActive |= (1 << (pin));
        }
        else {
            INTActive &= ~(1 << (pin));
        }
    }
    else if(port == IOPort::B) {
        if(enable) {
            PCINTActive |= (1 << (pin));
        }
        else {
            PCINTActive &= ~(1 << (pin));
        }
    }
}

inline static void genINTIRQ(uint8_t pin) {
    if((INTActive & (1 << pin)) && INTHandler[pin]) {
        INTHandler[pin]();
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

ISR(INT2_vect)
{
    genINTIRQ(2);
}

ISR(INT3_vect)
{
    genINTIRQ(3);
}

ISR(INT4_vect)
{
    genINTIRQ(4);
}

ISR(INT5_vect)
{
    genINTIRQ(5);
}

ISR(INT6_vect)
{
    genINTIRQ(6);
}

ISR(INT7_vect)
{
    genINTIRQ(7);
}

