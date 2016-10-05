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

#include "palTimer.h"

namespace cometos {


/**
 * Implementation of the PalTimer class for the STM32 Microcontroller.
 *
 * Until now, the class can handle the General Purpose Timers of the STM32F1,
 * which are timers 2-7. The extended functionality timers 0,1 and the timers
 * 8-14 are not implemented.
 *
 * For complete documentation of the member functions, see base class.
 *
 * TODO: Implement timers 0,1 and 8-14
 */
template <int periph>
class PalTimerImp : public PalTimer {

private:
	tim_callback_t callback;
	volatile bool delayMode;
	volatile uint16_t prescaler;
	volatile uint16_t soft_counter_up;
	volatile uint16_t soft_counter;

	volatile bool isAvailable;
	volatile bool isDelayMode;

public:

	void init(uint32_t freq);

	void start_async(uint16_t ticks, tim_callback_t callback);

	void setFrequency(uint32_t freq);

	void stop();

	void delay(uint16_t ticks);

	static PalTimerImp<periph>& getInstance();

	uint16_t get();

	bool available();

	void handleInterrupt();

protected:

	PalTimerImp();

	~PalTimerImp(){};

	/**
	 * 	Sets the counter to zero and the reload register to @a interval.
	 */
	void setupTimer(uint16_t interval);

	/**
	 *
	 */
	void startTimer(uint16_t interval);
};
}

