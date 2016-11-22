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

#include "palExec.h"
#include "palLocalTime.h"
#include "cometosAssert.h"

#ifdef __cplusplus
extern "C" {
#include "cmsis_device.h"
}
#endif

#define NO_POWERSAFE_MODE 0

static volatile time_ms_t localTime;
static volatile time_ms_t elapsedTime;
static volatile time_ms_t sleepCounter;
static volatile bool sleep;

void palLocalTime_init(){
	localTime = 0;
}

time_ms_t palLocalTime_get(){
	return localTime;
}

void palExec_init(){

	SystemCoreClockUpdate();

	//Set system tick to 1ms
	if (SysTick_Config( SystemCoreClock / 1000)){
		ASSERT(false);
		return; //TODO: Error Handling when SysTick_Config fails (shouldn't happen, though)
	}

	sleep = false;

	localTime = 0;
	elapsedTime = 0;
	sleepCounter = 0;
}

#ifdef __cplusplus
extern "C" {
#endif

void SysTick_Handler(void){

	if (sleep){
		if (sleepCounter == 0){
			sleep = false;
		}
		else{
			--sleepCounter;
		}
	}

	palExec_atomicBegin();
	++localTime;
	++elapsedTime;
	palExec_atomicEnd();
}

#ifdef __cplusplus
}
#endif


/**
 * Since power consumption is not a requirement here, do active waiting!
 * Uncomment content to use sleep
 */
void palExec_sleep(time_ms_t ms){
//  sleep = true;
//  sleepCounter = ms;
//
//  while (sleep){
//      __WFI();
//  }
}

void palExec_wakeup(){
	sleep = false;
}

static uint8_t inAtomic = 0;
static bool alreadyAtomic = false;

void palExec_atomicBegin() {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    ASSERT(inAtomic < UINT8_MAX);

    if(inAtomic == 0 && (primask & 1)) {
        // We are already in an atomic section (e.g. ISR)
	alreadyAtomic = true;
    }

    ++inAtomic;
}

void palExec_atomicEnd() {
    ASSERT(inAtomic >= 1);
    --inAtomic;

    if(inAtomic == 0) {
        if(alreadyAtomic) {
            alreadyAtomic = false;
        }
        else {
	    __enable_irq();
        }
    }
}

time_ms_t palExec_elapsed(){
	time_ms_t temporary;
	palExec_atomicBegin();
	temporary = elapsedTime;
	elapsedTime = 0;
	palExec_atomicEnd();
	return temporary;
}



