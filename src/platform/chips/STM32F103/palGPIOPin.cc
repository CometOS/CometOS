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
 * @author Peter Oppermann
 */

/*INCLUDES-------------------------------------------------------------------*/
#include "palGPIOPin.h"

namespace cometos {

IGPIOPinImpl::IGPIOPinImpl(GPIO_TypeDef* port, uint8_t pin){
	this->port = port;
	this->pin_mask = (1 << pin);
	this->pin = pin;
}

void IGPIOPinImpl::setDirection(Direction direction){
	dir = direction;

	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	if (direction == Direction::OUT){
		init.GPIO_Mode = GPIO_Mode_Out_PP;
	}

	init.GPIO_Pin = pin_mask;
	init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(port, &init);
}


uint8_t IGPIOPinImpl::get(){

	if ( dir == Direction::IN) {
		return port->IDR & pin_mask;
	}
	else {
		return port->ODR & pin_mask;
	}
}

void IGPIOPinImpl::set() {
	port->BSRR |= pin_mask;
}

void IGPIOPinImpl::clear() {
	port->BSRR |= (pin_mask << 16);
}

/**
 * Not implemented, since a general implementation of external interrupts is not
 * possible (easily) in ARM
 */
cometos_error_t IGPIOPinImpl::setupInterrupt(Edge type, Callback<void(void)> callback) {
	return COMETOS_ERROR_FAIL;
}

}

