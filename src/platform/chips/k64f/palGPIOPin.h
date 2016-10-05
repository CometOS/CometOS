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

#ifndef PALGPIOPIN
#define PALGPIOPIN

#include "IGPIOPin.h"
#include "cometos.h"

#include "device/fsl_device_registers.h"

namespace cometos {

#define MMIO32(x) (*(volatile uint32_t*)(x))
//#define PORTIDX(x) (((x)-PORTA)/(PORTB-PORTA))	 /* map PORTA,PORTB,... to 0,1,... */
#define PORTIDX(x) (x)	 /* map 0,1,... to 0,1,... */
#define SCGC5_MASK(x) (1<<((PORTIDX(x))+9))
#define PDOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PDOR))    /* output register address of x */
#define PSOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PSOR))    /* set register address of x */
#define PCOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PCOR))    /* clear register address of x */
#define PTOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PTOR))    /* toggle register address of x */
#define PDIR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PDIR))    /* input register address of x */
#define PDDR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PDDR))    /* data direction register address of x */
#define PCR(x,p) MMIO32(((PORTIDX(x))*0x400) + &PORTA_PCR0 + (p)) /* pin control register */
#define PISFR(x) MMIO32((((PORTIDX(x))*0x400) + &PORTA_ISFR))    /* interrupt flag register */

enum class IOPort : uint8_t {
        A = 0, B = 1, C = 2, D = 3, E = 4
};

extern Callback<void(void)> interruptHandler[5][32];
extern uint32_t interruptActivated[5];

template<IOPort port, uint8_t pin>
class GPIOPin : public IGPIOPin {
private:
        Direction currentDirection;

public:
        virtual void setDirection(Direction direction) {
		SIM_SCGC5 |= SCGC5_MASK((uint8_t)port); 
		PCR((uint8_t)port,pin) = 0x100;

                currentDirection = direction;
                if(direction == Direction::IN) {
			PDDR((uint8_t)port) &= ~(1 << (pin));
                }
                else {
			PDDR((uint8_t)port) |= (1 << (pin));
                }
        }

        virtual void interruptActive(bool enable) {
            if(enable) {
		        interruptActivated[(uint8_t)port] |= (1 << (pin));
            }
            else {
		        interruptActivated[(uint8_t)port] &= ~(1 << (pin));
            }
               
        }

	virtual cometos_error_t setupInterrupt(Edge type, Callback<void(void)> callback) {
		interruptActivated[(uint8_t)port] |= (1 << (pin));
		interruptHandler[(uint8_t)port][pin] = callback;

		PCR((uint8_t)port,pin) &= ~(0xF << 16);
		uint8_t val = 0;
		switch(type) {
		case Edge::NONE:
			val = 0x0;
			break;
		case Edge::RISING:
			val = 0x9;
			break;
		case Edge::FALLING:
			val = 0xA;
			break;
		case Edge::ANY_EDGE:
			val = 0xB;
			break;
		case Edge::LOW_LEVEL:
			val = 0x8;
			break;
		case Edge::HIGH_LEVEL:
			val = 0xC;
			break;
		}
		PCR((uint8_t)port,pin) |= (val << 16);

		switch(port) {
		case IOPort::A:
			NVIC_EnableIRQ(PORTA_IRQn);
			break;
		case IOPort::B:
			NVIC_EnableIRQ(PORTB_IRQn);
			break;
		case IOPort::C:
			NVIC_EnableIRQ(PORTC_IRQn);
			break;
		case IOPort::D:
			NVIC_EnableIRQ(PORTD_IRQn);
			break;
		case IOPort::E:
			NVIC_EnableIRQ(PORTE_IRQn);
			break;
		}

                return COMETOS_SUCCESS;
        }

        virtual uint8_t get() {
            if(currentDirection == Direction::IN) {
		return ((PDIR((uint8_t)port)&(1 << pin))>0);
            }
            else {
		return (1&(PDOR((uint8_t)port) >> (pin)));
            }
        }

        virtual void set() {
		PSOR((uint8_t)port) = (1 << (pin));
        }

        virtual void clear() {
		PCOR((uint8_t)port) = (1 << (pin));
        }

	virtual void pullup(bool enable) {
		if(enable) {
			PCR((uint8_t)port,pin) |= 0x3;
		}
		else {
			PCR((uint8_t)port,pin) &= ~0x3;
		}
	}
};

}

#endif
