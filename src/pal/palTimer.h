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

#ifndef PALTIMER_H_
#define PALTIMER_H_

#include <stdint.h>

//-------------- TYPEDEFS -------------------
typedef void (* tim_callback_t) (void);


/**
 * Abstract definition of a timer.
 */
namespace cometos {

enum class Timer {
UART,RADIO,RADIO_ALARM,GENERAL_PURPOSE_A,GENERAL_PURPOSE_B
};

class PalTimer {

public:

	/**
	 *Initializes the timer and configures the prescaler
	 * such that the counter increases with a frequency of
	 * freq
	 *
	 * @param freq The counter increment frequency
	 */
	virtual void init(uint32_t freq) = 0;

	/**
	 * Starts the timer in the background counting up from 0 to
	 * ticks. When the counter reaches ticks, a the callback function
	 * is being executed
	 */
	virtual void start_async(uint16_t ticks, tim_callback_t callback) = 0;

	/**
	 * Sets the timer increment frequency to freq.
	 */
	virtual void setFrequency(uint32_t freq) = 0;

	/**
	 * Starts the timer counting up from 0 to ticks,
	 * synchronously waiting (blocking) until it reaches
	 * ticks.
	 */
	virtual void delay(uint16_t ticks) = 0;

	/**
	 * Stops a currently running timer.
	 */
	virtual void stop() = 0;

	/**
	 * Returns an instance of an implementation of timer. Periph
	 * is the timer index, in case there are several timers available
	 * on the platform.
	 */
	static PalTimer* getInstance(Timer periph);

	/**
	 * Yields the current usage status of the timer.
	 *
	 * @return true, if the timer is not currently running, false else.
	 */
	virtual bool available() = 0;

	/**
	 * Yields the current counter value.
	 *
	 */
	virtual uint16_t get() = 0;

protected:

	/**
	 * Every subclass should enforce singleton. Therefore the constructor
	 * can only be called from subclasses.
	 */
	PalTimer(){};

	virtual ~PalTimer() {};
};

}

#endif //PALTIMER_H_
