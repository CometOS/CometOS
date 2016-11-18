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

MacSymbolCounter& MacSymbolCounter::getInstance() {
    static MacSymbolCounter msc;
    return msc;
}

void MacSymbolCounter::init(Callback<void()> compareMatch) {
    this->compareMatch = compareMatch;

	RCC_ClocksTypeDef clocks;
	RCC_GetClocksFreq(&clocks);

    // TODO calculate correct prescaler
    uint32_t freq = 1000;
	uint32_t full_pre = clocks.PCLK1_Frequency * 2 / (uint32_t)freq;

	uint32_t soft_counter_up = full_pre / 65536;
	uint32_t prescaler = full_pre / (soft_counter_up + 1);

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

void TIM3_IRQHandler(void)
{
	if (TIM3->SR & TIM_SR_UIF){
		TIM3->SR &= (uint16_t) ~TIM_SR_UIF;
		//cometos::PalTimerImp<3>::getInstance().handleInterrupt();
        cometos::getCout() << "." << endl;
	}
}
