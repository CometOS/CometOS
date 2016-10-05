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

#ifndef PALRAND_C
#define PALRAND_C

#include "rf231.h"
#include "palTimer.h"
#include "palRand.h"
#include "cmsis_device.h"

static unsigned int cometos_rnd = 0;

unsigned int palRand_get() {
	return cometos_rnd;
}

void palRand_init() {
	cometos::Rf231 * rf = cometos::Rf231::getInstance();
	cometos::PalTimer* timer = cometos::PalTimer::getInstance(4);
	timer->setFrequency(1e6);

	rf->cmd_state(AT86RF231_TRX_STATE_FORCE_TRX_OFF);

	while (rf->getRfStatus() != AT86RF231_TRX_STATUS_TRX_OFF) {
		__asm("nop");
	}

	rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);

	while (rf->getRfStatus() != AT86RF231_TRX_STATUS_RX_ON)
		__asm("nop");

	unsigned int rnd = 0;

	//the random value is updated every 1 us in the rf231
	timer->delay(1);

	for (uint8_t i=0; i < sizeof(rnd) * 8 / 2; i++) {
		timer->delay(1);
		uint8_t rssiReg = rf->readRegister(AT86RF231_REG_PHY_RSSI);
		rnd = (rnd << 2) | ((rssiReg >> AT86RF231_PHY_RSSI_RND_0) & 0x03);
	}

	rf->cmd_state(AT86RF231_TRX_STATE_TRX_OFF);

	cometos_rnd = rnd;

	while (rf->getRfStatus() != AT86RF231_TRX_STATUS_TRX_OFF)
		__asm("nop");
}

#endif //PALRAND_C
