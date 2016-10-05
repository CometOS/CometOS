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

#include "palRand.h"
#include "device/fsl_device_registers.h"

// taken from https://developer.mbed.org/questions/5374/Random-numbers-on-K64F-stuck-at-executio/

static unsigned int cometos_rnd = 0;

unsigned int palRand_get() {
    return cometos_rnd;
}

void palRand_init() {
    /* turn on RNGA module */
    SIM_SCGC6 |= SIM_SCGC6_RNGA_MASK;

    /* set SLP bit to 0 - "RNGA is not in sleep mode" */
    RNG_CR &= ~RNG_CR_SLP_MASK; 
 
    /* set HA bit to 1 - "security violations masked" */
    RNG_CR |= RNG_CR_HA_MASK;
 
    /* set GO bit to 1 - "output register loaded with data" */
    RNG_CR |= RNG_CR_GO_MASK;
 
    /* wait for RNG FIFO to be full */
    while((RNG_SR & RNG_SR_OREG_LVL(0xF)) == 0) {}
 
    /* get value */
    cometos_rnd = RNG_OR;
}


