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
#include <avr/io.h>

namespace cometos {

enum class IOPort : uint8_t {
        B = 0, C = 1, D = 2
};

class GPIOImpl {
public:
	static cometos_error_t setupInterrupt(IOPort port, uint8_t pin, IGPIOPin::Edge type, Callback<void(void)> callback);
        static void interruptActive(IOPort port, uint8_t pin, bool enable);
};

template<IOPort port, uint8_t pin>
class GPIOPin : public IGPIOPin {
private:
        constexpr static volatile uint8_t* PORT_ADDR = (&PORTB)+((&PORTC)-(&PORTB))*(uint8_t)port;
        constexpr static volatile uint8_t* DDR_ADDR = PORT_ADDR-1;
        constexpr static volatile uint8_t* PIN_ADDR = PORT_ADDR-2;

        Direction currentDirection;

public:
        virtual void setDirection(Direction direction) {
                currentDirection = direction;
                if(direction == Direction::IN) {
                        *DDR_ADDR &= ~(1 << pin);
                }
                else {
                        *DDR_ADDR |= (1 << pin);
                }
        }

	virtual cometos_error_t setupInterrupt(Edge type, Callback<void(void)> callback) {
                return GPIOImpl::setupInterrupt(port,pin,type,callback);
        }

        virtual void interruptActive(bool enable) {
                GPIOImpl::interruptActive(port,pin,enable);
        }

        virtual uint8_t get() {
            if(currentDirection == Direction::IN) {
                return ((*PIN_ADDR) >> pin) & 1;
            }
            else {
                return ((*PORT_ADDR) >> pin) & 1;
            }
        }

        virtual void set() {
                *PORT_ADDR |= (1 << pin);
        }

        virtual void clear() {
                *PORT_ADDR &= ~(1 << pin);
        }

	virtual void pullup(bool enable) {
		// enable pullup is the same as setting the port
		if(enable) {
			set();
		}
		else {
			clear();
		}
	}
};


}

#endif
