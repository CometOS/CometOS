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

/**
 * Based on Texas Instruments CC11xL Api
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CC1190Api.h"
#include "cc_common.h"

namespace cometos {

void CC1190Api::init(void)
{
    DDR(CC1190_CTRL_PORT) |= ((1<<CC1190_PA_EN)|(1<<CC1190_LNA_EN)|(1<<CC1190_HGM));
    CC1190_CTRL_PORT &= ~((1<<CC1190_PA_EN)|(1<<CC1190_LNA_EN)|(1<<CC1190_HGM));

    cc1190Status = CC1190_DISABLED;
}

void CC1190Api::enable(void)
{
    cc1190Status |= (1<<CC1190_SET_EN);
}

void CC1190Api::disable(void)
{
    CC1190_CTRL_PORT &= ~((1<<CC1190_PA_EN)|(1<<CC1190_LNA_EN)|(1<<CC1190_HGM));
    cc1190Status = CC1190_DISABLED;
}

void CC1190Api::highGainEnable(void)
{
    CC1190_CTRL_PORT |= (1<<CC1190_HGM);
    cc1190Status |= (1<<CC1190_SET_HG);
}

void CC1190Api::highGainDisable(void)
{
    CC1190_CTRL_PORT &= ~(1<<CC1190_HGM);
    cc1190Status &= ~(1<<CC1190_SET_HG);
}

void CC1190Api::enterRX(void)
{
    CC1190_CTRL_PORT &= ~(1<<CC1190_PA_EN);
    CC1190_CTRL_PORT |= (1<<CC1190_LNA_EN);
    cc1190Status &= ~(1<<CC1190_SET_RXTX);
}

void CC1190Api::enterTX(void)
{
    CC1190_CTRL_PORT |= (1<<CC1190_PA_EN);
    CC1190_CTRL_PORT &= ~(1<<CC1190_LNA_EN);
    cc1190Status |= (1<<CC1190_SET_RXTX);
}

} /* namespace cometos */
