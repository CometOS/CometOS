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

#include "palTimerImp.h"
#include "cmsis_device.h"
#include "cometosAssert.h"
#include "palExec.h"

#define TIM_RCC_FLAG(X) (RCC_APB1Periph_TIM2 << (X - 2))
#define TIM_BASE(X) ((TIM_TypeDef*) (((uint32_t) TIM2) + 0x0400 * (X - 2)))
#define DISABLE_INTERRUPT(X) (TIM_BASE(X))->DIER &= ~TIM_DIER_UIE
#define ENABLE_INTERRUPT(X) (TIM_BASE(X))->DIER |= TIM_DIER_UIE
#define CLEAR_UPDATE_INTERRUPT(X)  (TIM_BASE(X))->SR &= ~TIM_SR_UIF

namespace cometos {

template <int Peripheral>
void PalTimerImp<Peripheral>::init(uint32_t freq) {
	setFrequency(freq);
}

template <int Peripheral>
void PalTimerImp<Peripheral>::setFrequency(uint32_t freq){

	RCC_ClocksTypeDef clocks;
	RCC_GetClocksFreq(&clocks);

	uint32_t full_pre = clocks.PCLK1_Frequency * 2 / (uint32_t)freq;

	soft_counter_up = full_pre / 65536;
	prescaler = full_pre / (soft_counter_up + 1);

	/**
	 * IRQ Lines don't follow a pattern, have to be
	 * manually defined here.
	 */
	uint8_t irqLine;
	switch (Peripheral){
	case 2:
		irqLine = TIM2_IRQn;
		break;
	case 3:
		irqLine = TIM3_IRQn;
		break;
	case 4:
		irqLine = TIM4_IRQn;
		break;
	case 5:
		irqLine = TIM5_IRQn;
		break;
	case 6:
		irqLine = TIM6_IRQn;
		break;
	case 7:
		irqLine = TIM7_IRQn;
		break;
	default:
		break;
	}

	// Init interrupt line
	NVIC_InitTypeDef nvicInit;
	nvicInit.NVIC_IRQChannel = irqLine;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 0xF;
	nvicInit.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_Init(&nvicInit);

	//Activate clock for timer module
	RCC_APB1PeriphClockCmd(TIM_RCC_FLAG(Peripheral), ENABLE);

	//Set Registers
	TIM_BASE(Peripheral)->PSC = prescaler;
	TIM_BASE(Peripheral)->EGR = TIM_PSCReloadMode_Immediate;
}

template <int Peripheral>
inline void PalTimerImp<Peripheral>::setupTimer(uint16_t interval){
	TIM_BASE(Peripheral)->ARR = interval;
	TIM_BASE(Peripheral)->CNT = 0;
	TIM_BASE(Peripheral)->EGR = TIM_PSCReloadMode_Immediate;
}

template <int Peripheral>
void PalTimerImp<Peripheral>::startTimer(uint16_t interval){

	//Disable interrupt during setup
	DISABLE_INTERRUPT(Peripheral);

	setupTimer(interval);

	delayMode = false;
	soft_counter = 0;

	//Enable Timer
	CLEAR_UPDATE_INTERRUPT(Peripheral);
	ENABLE_INTERRUPT(Peripheral);
	TIM_BASE(Peripheral)->CR1 |= TIM_CR1_CEN;
}

template <int Peripheral>
void PalTimerImp<Peripheral>::start_async(uint16_t ticks, tim_callback_t callback){
	DISABLE_INTERRUPT(Peripheral);
	stop();

	isAvailable = false;
	this->callback = callback;

	startTimer(ticks);
}

template <int Peripheral>
bool PalTimerImp<Peripheral>::available(){
	return isAvailable;
}

template <int Peripheral>
inline void PalTimerImp<Peripheral>::stop() {
	palExec_atomicBegin();

	TIM_BASE(Peripheral)->CR1 &= ~TIM_CR1_CEN;
	CLEAR_UPDATE_INTERRUPT(Peripheral);

	isAvailable = true;
	palExec_atomicEnd();
}

template <int Peripheral>
void PalTimerImp<Peripheral>::delay(uint16_t ticks){

	DISABLE_INTERRUPT(Peripheral);

	isAvailable = false;

	//Setup Timer
	setupTimer(ticks + 1);

	delayMode = true;
	soft_counter = 0;

	CLEAR_UPDATE_INTERRUPT(Peripheral);
	TIM_BASE(Peripheral)->CR1 |= TIM_CR1_CEN;

	while ( TIM_BASE(Peripheral)->CNT < ticks ) {
		__asm("nop");
	}

	TIM_BASE(Peripheral)->CR1 &= ~TIM_CR1_CEN;
	isAvailable = true;
}

template <int Peripheral>
PalTimerImp<Peripheral>& PalTimerImp<Peripheral>::getInstance(){
	static PalTimerImp<Peripheral> tim;
	return tim;
}


template <int Peripheral>
uint16_t PalTimerImp<Peripheral>::get(){
	return TIM_BASE(Peripheral)->CNT;
}

template <int Peripheral>
void PalTimerImp<Peripheral>::handleInterrupt() {

	palExec_atomicBegin();
	DISABLE_INTERRUPT(Peripheral);

	if(soft_counter < soft_counter_up){
		soft_counter++;
		ENABLE_INTERRUPT(Peripheral);
		palExec_atomicEnd();
		return;
	}

	stop();

	palExec_atomicEnd();

	if (delayMode)
		delayMode = false;
	else if (callback != 0){
		callback();
	}
}

template <int Peripheral>
PalTimerImp<Peripheral>::PalTimerImp() {
	callback = 0;
	prescaler = 72;
	soft_counter_up = 0;
	soft_counter = 0;
	isAvailable = true;
}



PalTimer* PalTimer::getInstance(Timer i){
	switch (i){
    case Timer::UART:
		return &(PalTimerImp<2>::getInstance());
    //case 3: USED IN MacSymbolCounter.h/.cc
	//  return &(PalTimerImp<3>::getInstance());
    case Timer::RADIO:
		return &(PalTimerImp<4>::getInstance());
	case Timer::RADIO_ALARM:
		return &(PalTimerImp<5>::getInstance());
    case Timer::GENERAL_PURPOSE_A:
		return &(PalTimerImp<6>::getInstance());
    case Timer::GENERAL_PURPOSE_B:
		return &(PalTimerImp<7>::getInstance());
	default:
		break;
	}

	return 0;
}

//Explicitly declare the needed template classes for the compiler

template class PalTimerImp<2>;
template class PalTimerImp<3>;
template class PalTimerImp<4>;
template class PalTimerImp<5>;
template class PalTimerImp<6>;
template class PalTimerImp<7>;


}


extern "C" {

void TIM2_IRQHandler(void)
{
	if (TIM2->SR & TIM_SR_UIF){
		TIM2->SR &= (uint16_t) ~TIM_SR_UIF;
		cometos::PalTimerImp<2>::getInstance().handleInterrupt();
	}
}

/* USED IN MacSymbolCounter.cc/.h
void TIM3_IRQHandler(void)
{
	if (TIM3->SR & TIM_SR_UIF){
		TIM3->SR &= (uint16_t) ~TIM_SR_UIF;
		cometos::PalTimerImp<3>::getInstance().handleInterrupt();
	}
}
*/

void TIM4_IRQHandler(void)
{
	if (TIM4->SR & TIM_SR_UIF){
		TIM4->SR &= (uint16_t) ~TIM_SR_UIF;
		cometos::PalTimerImp<4>::getInstance().handleInterrupt();
	}
}

void TIM5_IRQHandler(void)
{
	if (TIM5->SR & TIM_SR_UIF){

		//Clear ITPending bit
		TIM5->SR &= (uint16_t) ~TIM_SR_UIF;

		cometos::PalTimerImp<5>::getInstance().handleInterrupt();
	}
}

void TIM6_IRQHandler(void)
{
	if (TIM6->SR & TIM_SR_UIF){
		TIM6->SR &= (uint16_t) ~TIM_SR_UIF;

		cometos::PalTimerImp<6>::getInstance().handleInterrupt();
	}
}

void TIM7_IRQHandler(void)
{
	if (TIM7->SR & TIM_SR_UIF){
		TIM7->SR &= (uint16_t) ~TIM_SR_UIF;

		cometos::PalTimerImp<7>::getInstance().handleInterrupt();
	}
}

}




