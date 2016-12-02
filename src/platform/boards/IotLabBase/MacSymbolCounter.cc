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

#include "MacSymbolCounter.h"
#include "cmsis_device.h"
#include "cometosAssert.h"
#include "OutputStream.h"

using namespace cometos;

static MacSymbolCounter msc;

MacSymbolCounter& MacSymbolCounter::getInstance() {
    return msc;
}

void MacSymbolCounter::init(Callback<void()> compareMatch) {
    this->compareMatch = compareMatch;
    lastCapture = 0;

	//Disable interrupt during setup
    TIM3->DIER &= ~TIM_DIER_UIE;

	RCC_ClocksTypeDef clocks;
	RCC_GetClocksFreq(&clocks);

    // TODO calculate correct prescaler
    //uint32_t freq = 65536;
    uint32_t freq = 62500;
	uint32_t prescaler = clocks.PCLK1_Frequency * 2 / (uint32_t)freq;

	// Init interrupt line
	NVIC_InitTypeDef nvicInit;
	nvicInit.NVIC_IRQChannel = TIM3_IRQn;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 0xF;
	nvicInit.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_Init(&nvicInit);

	//Activate clock for timer module
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	//Set Registers
	TIM3->PSC = prescaler;
	TIM3->EGR = TIM_PSCReloadMode_Immediate;

	TIM3->ARR = 0xFFFF;
	TIM3->CNT = 0;

	TIM3->CR1 &= ~TIM_CR1_DIR; // upcounting

    // Enable capture
    TIM3->CCMR2 &= ~TIM_CCMR2_CC3S;
    TIM3->CCMR2 |= TIM_CCMR2_CC3S_0; // enable input
    TIM3->CCMR2 &= ~TIM_CCMR2_IC3PSC; // disable input prescaler
    TIM3->CCMR2 &= ~TIM_CCMR2_IC3F; // disable input filter
    TIM3->CCER &= ~TIM_CCER_CC3P; // rising edge
    TIM3->CCER |= TIM_CCER_CC3E; // enable capture

    // Enable Interrupts
    TIM3->SR &= ~TIM_SR_UIF; // clear overflow interrupt
    TIM3->DIER |= TIM_DIER_UIE; // enable overflow interrupt
    TIM3->SR &= ~TIM_SR_CC1IF; // clear compare match interrupt
    TIM3->DIER |= TIM_DIER_CC1IE; // enable compare match interrupt
    TIM3->SR &= ~TIM_SR_CC3IF; // clear capture interrupt
    TIM3->DIER |= TIM_DIER_CC3IE; // enable capture interrupt

	//Enable Timer
	TIM3->CR1 |= TIM_CR1_CEN;
}

void MacSymbolCounter::interrupt() {
    bool overflow = false;

	if(TIM3->SR & TIM_SR_UIF){
		TIM3->SR &= (uint16_t) ~TIM_SR_UIF;
        overflow = true;
        msw++;
	}

    if(TIM3->SR & TIM_SR_CC1IF) {
		TIM3->SR &= (uint16_t) ~TIM_SR_CC1IF;

        uint16_t compareMSW = msw;

        if(overflow && TIM3->CCR1 >= 0x8000) {
            // The compare match interrupt was probably
            // before the overflow interrupt
            compareMSW--;
        }

        if(compareMSW == compareValueMSW) {
            compareMatch();
        }
    }
    
    if(TIM3->SR & TIM_SR_CC3IF) {
		TIM3->SR &= (uint16_t) ~TIM_SR_CC3IF;

        uint16_t captureLSW = TIM3->CCR3;
        uint16_t captureMSW = msw;

        if(overflow && captureLSW >= 0x8000) {
            // The capture interrupt was probably
            // before the overflow interrupt
            captureMSW--;
        }

        lastCapture = (captureMSW << (uint32_t)16) | captureLSW;
    }

    ASSERT((TIM3->SR & 0xFF00) == 0); // no overcapture
}

uint32_t MacSymbolCounter::getValue() {
    palExec_atomicBegin();
    uint32_t result = (((uint32_t)msw) << 16) | TIM3->CNT;
    if(TIM3->SR & TIM_SR_UIF) {
        // Recent and unhandled overflow
        result += (1 << (uint32_t)16);
    }
    palExec_atomicEnd();
    return result;
}

uint32_t MacSymbolCounter::getCapture() {
    palExec_atomicBegin();
    uint32_t result = lastCapture;
    palExec_atomicEnd();
    return result;
}

void MacSymbolCounter::setCompareMatch(uint32_t compareValue) {
    palExec_atomicBegin();
    compareValueMSW = compareValue >> 16;
    TIM3->CCR1 = compareValue & 0xFFFF;
    palExec_atomicEnd();
}

extern "C" {

void TIM3_IRQHandler(void)
{
    msc.interrupt();
}

}
