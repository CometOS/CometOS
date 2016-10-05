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

#include "IGPIOPin.h"
#include "cmsis_device.h"

namespace cometos {

/*PROTOTYPES-----------------------------------------------------------------*/

/**
 * This class is IGPIOPins implementation for the STM32 microcontroller.
 * Not all functions of the base class could be implemented as specified.
 * Interrupts can not be set with the given implementation, since on ARM
 * architecture, the PinInterrupt are somewhat more complicated than on AVR.
 */

class IGPIOPinImpl : public IGPIOPin{

private:
	GPIO_TypeDef* port;
	uint16_t pin_mask;
	uint8_t pin;

	IGPIOPin::Direction dir;

//	Callback<void(void)> callback;

public:

	IGPIOPinImpl(GPIO_TypeDef* port, uint8_t pin);

	void setDirection(Direction direction);

	/**
	 * Setup GPIO interrupt and activates it.
	 * It will override a previous setup on the same port and pin.
	 *
	 * TODO: not implemented yes
	 *
	 * @param port Port of the pin
	 * @param pin Pin
	 * @param type Edge of interrupt
	 * @param callback Callback that is called from interrupt context
	 * @return COMETOS_ERROR_INVALID if type is not available, COMETOS_SUCCESS otherwise
	 */
	cometos_error_t setupInterrupt(Edge type, Callback<void(void)> callback);

	/**
	 * Activates or deactivates a previously set up interrupt.
	 */
	void interruptActive(bool enable){};

	uint8_t get();

	void set();
	void clear();

	void pullup(bool enable) {};
};

}
