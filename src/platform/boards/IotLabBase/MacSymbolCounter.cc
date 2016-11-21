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
    TIM3->CCER |= TIM_CCER_CC1E; // enable capture

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
		//cometos::PalTimerImp<3>::getInstance().handleInterrupt();
        msb++;
        getCout() << msb << endl;
	}

    if(TIM3->SR & TIM_SR_CC1IF) {
		TIM3->SR &= (uint16_t) ~TIM_SR_CC1IF;

        uint8_t compareMSB = msb;

        if(overflow && TIM3->CCR1 >= 0x8000) {
            // The compare match interrupt was probably
            // before the overflow interrupt
            compareMSB--;
        }

        if(compareMSB == compareValueMSB) {
            compareMatch();
        }
    }
    
    if(TIM3->SR & TIM_SR_CC3IF) {
		TIM3->SR &= (uint16_t) ~TIM_SR_CC3IF;

        uint8_t captureLSB = TIM3->CCR3;
        uint8_t captureMSB = msb;

        if(overflow && captureLSB >= 0x8000) {
            // The capture interrupt was probably
            // before the overflow interrupt
            captureMSB--;
        }

        lastCapture = (captureMSB << (uint32_t)16) | captureLSB;
    }

    ASSERT((TIM3->SR & 0xFF00) == 0); // no overcapture
}

uint32_t MacSymbolCounter::getValue() {
    return (((uint32_t)msb) << 16) | TIM3->CNT;
}

uint32_t MacSymbolCounter::getCapture() {
    return lastCapture;
}

void MacSymbolCounter::setCompareMatch(uint32_t compareValue) {
    compareValueMSB = compareValue >> 16;
    TIM3->CCR1 = compareValue & 0xFFFF;
}

/*
template <int Peripheral>
inline void PalTimerImp<Peripheral>::initInputCapture(){
	// Channel 3 Configuration in InputCapture
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;

	TIM_ICInit(TIM_BASE(Peripheral), &TIM_ICInitStructure);
}
*/

extern "C" {

void TIM3_IRQHandler(void)
{
    msc.interrupt();
}

}
