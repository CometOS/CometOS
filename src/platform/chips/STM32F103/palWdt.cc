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

#include "palWdt.h"

#include "cometosAssert.h"
#include "cmsis_device.h"
#include "palExec.h"

#include <stdbool.h>

/**
 * STM32 has to types of watchdogs: The Window Watchdog and the independent
 * watchdog.
 *
 * Both are implemented, but each one has it's drawbacks.
 *
 *  The window watchdog has a very small maximum timeout (~58ms). Therefore
 *  a software counter has been implemented to be counted up on wwdg-timeouts.
 *  However, when the cpu does not serve the wd-interrupt soon enogh, the device
 *  resets too early.
 *
 *  The Independed watchdog has sufficiently long timeout periods, but no interrupt.
 *  The header of palWdt, however, specifies a callback function that can be given
 *  to the watchdog. This can not be done with the independet watchdog. Also, the
 *  independent watchdog can not be paused, once it was activated once.
 */
#define USE_WWDG 0

#if USE_WWDG
/** LOCAL VARIABLES ******************/
static palWdt_callback callback;
static uint8_t counter_reset_value;



/* soft counter needed, since maximum hardware timeout is 58ms */
static volatile uint16_t soft_counter;
static uint16_t soft_counter_top;

static bool initialized = false;



/** FUNCTION IMPLEMENTATIONS ********/

uint16_t palWdt_enable(uint16_t timeout, palWdt_callback cb){

	callback = cb;

	RCC_ClocksTypeDef clocks;
	RCC_GetClocksFreq(&clocks);

	//WWDG is powered by pclk 1, enable clock!
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);

	WWDG_SetPrescaler(WWDG_Prescaler_8);

	uint32_t resetValue;

	//formula to calculate timeout from the datasheet p. 493
	uint32_t pclk1_freq_per_ms = (clocks.PCLK1_Frequency / 1000);
	resetValue = ((uint64_t) timeout * pclk1_freq_per_ms) / (4096 * 8) - 1;

	//configure soft counter
	if (resetValue >= 0x40) {
		soft_counter_top = resetValue / 0x3F;
		resetValue = 0x3F;
	}
	else {
		soft_counter_top = 0;
	}
	soft_counter = 0;

	//set bit 6 (WD fires, if bit 6 flips to 0)
	resetValue |= (1 << 6);

	//Window value is maximum, since we do ignore "too early" resets
	WWDG_SetWindowValue((uint8_t) 0x7F);
	WWDG_ClearFlag();

	NVIC_InitTypeDef nvic_init;
	nvic_init.NVIC_IRQChannel = WWDG_IRQn;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	nvic_init.NVIC_IRQChannelPreemptionPriority= 0x02;
	nvic_init.NVIC_IRQChannelSubPriority = 0x03;
	NVIC_Init(&nvic_init);

	NVIC_EnableIRQ(WWDG_IRQn);
	WWDG_EnableIT();

	counter_reset_value = resetValue | 0x40; //Reset the bit triggering the interrupt as well

	//Enable
	WWDG_Enable(counter_reset_value);

	initialized = true;

	//return real timespan
	return timeout;
}


bool palWdt_isRunning() {

	if ((WWDG_GetFlagStatus() & WWDG_CR_WDGA) )
		return true;

	return false;
}

/**
 * Resets the watchdog timer. Needs to be called regularly to prevent watchdog
 * from firing.
 */
void palWdt_reset(){

	if (!initialized)
		return;

	WWDG_SetCounter(counter_reset_value);
	soft_counter = 0;
}

/**
 * Disable the watchdog temporarily.
 */
void palWdt_pause(){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, DISABLE);
}

void palWdt_resume(){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);
}

extern "C" void WWDG_IRQHandler() {

	palExec_atomicBegin();
	WWDG_ClearFlag();

	//if a soft counter is used, check for top condition
	if (soft_counter < soft_counter_top) {
		WWDG_SetCounter(counter_reset_value);
		soft_counter++;
		palExec_atomicEnd();
		return;
	}

	palExec_atomicEnd();

	if (callback != 0)
		callback();
}

#else

#define IWDG_REGISTER_ACCESS_CODE 0x5555
#define IWDG_RELOAD_COUNTER_CODE 0xAAAA
#define IWDG_START_CODE 0xCCCC
#define IWDG_PRESCALER_REG_VALUE 0x4
#define LSI_CLK_FREQ 40000

static bool isRunning = false;

uint16_t palWdt_enable(uint16_t timeout, palWdt_callback cb){

    uint32_t counterFreq = LSI_CLK_FREQ / (1 << (2 + IWDG_PRESCALER_REG_VALUE));
    uint32_t resetValue = timeout * counterFreq / 1000;

    IWDG_TypeDef* iwdg = (IWDG_TypeDef*) IWDG_BASE;

    //Enable wdg stop on dbg!
    ((DBGMCU_TypeDef*) DBGMCU_BASE)->CR |= DBGMCU_IWDG_STOP;

    iwdg->KR = IWDG_REGISTER_ACCESS_CODE;
    iwdg->PR = IWDG_PRESCALER_REG_VALUE;

    iwdg->KR = IWDG_REGISTER_ACCESS_CODE;
    iwdg->RLR = resetValue;

    iwdg->KR = IWDG_START_CODE;

    palWdt_reset();

    isRunning = true;

    return timeout;
}


bool palWdt_isRunning() {
    return isRunning;
}

/**
 * Resets the watchdog timer. Needs to be called regularly to prevent watchdog
 * from firing.
 */
void palWdt_reset(){
    IWDG_TypeDef* iwdg = (IWDG_TypeDef*) IWDG_BASE;
    iwdg->KR = IWDG_RELOAD_COUNTER_CODE;
}

/**
 * Disable the watchdog temporarily. Not available with iwdg
 */
void palWdt_pause(){

}

/**
 * Resume the disabled watchdog. Not available with iwdg
 */
void palWdt_resume(){
}

#endif
