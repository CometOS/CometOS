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

#include "palAES.h"

#include "aes.h"

static uint8_t keyStorage[PAL_AES_BYTES];

namespace cometos {

Arbiter PalAES::arbiter;
Callback<void(uint8_t*)> encCallback;
void encReady();
SimpleTask encReadyTask(encReady);
uint8_t* result;
bool busy = false;

cometos_error_t PalAES::setKey(const uint8_t *key) {
    ASSERT_RUNNING(&arbiter);

    if(busy) {
	return COMETOS_ERROR_BUSY;
    }

    memcpy(keyStorage, key, PAL_AES_BYTES);

    return COMETOS_SUCCESS;
}

cometos_error_t PalAES::encrypt(uint8_t *inAndOut, Callback<void(uint8_t*)> callback) {
    ASSERT_RUNNING(&arbiter);
    
    if(busy) {
	return COMETOS_ERROR_BUSY;
    }
    busy = true;

    AES128_ECB_encrypt(inAndOut, keyStorage);

    result = inAndOut;
    encCallback = callback;

    getScheduler().add(encReadyTask);

    return COMETOS_PENDING;
}

void encReady() {
    busy = false;
    encCallback(result);
}

Arbiter* PalAES::getArbiter() {
    return &arbiter;
}

}
