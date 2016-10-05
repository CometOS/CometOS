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

#ifndef BROADCAST_BEACON_H
#define BROADCAST_BEACON_H

#include <stdint.h>
#include "palAES.h"
#include "OutputStream.h"
#include "Arbiter.h"
#include "cometosError.h"
#include "CC110lApi.h"

#define NUM_CHANNELS 4

class BroadcastBeacon {
public:
    typedef uint64_t timestamp_t;

    static const uint8_t BEACON_TIMESTAMP_BYTES = 6;
    static const uint8_t BEACON_MAC_BYTES = 6;

    BroadcastBeacon();
    cometos_error_t setTime(timestamp_t timestamp);  
    timestamp_t getTime();  
    cometos_error_t setKey(const uint8_t* key);
    cometos_error_t generateMIC(cometos::Callback<void()> callback);
    bool isMICUpdated();
    cometos_error_t setData(uint8_t length, const uint8_t* data);
    const uint8_t* getData() const;
    void print(cometos::OutputStream* outputStream = NULL) const;
    bool isMICMatching(BroadcastBeacon& other);

    static uint8_t getSize();
    static uint16_t getIntervalMS();
    static uint16_t getPreambleSyncShift();
    static void setupTransceiver(cometos::CC110lApi& ccApi);

private:
    uint8_t fullMAC[PAL_AES_BYTES];

    struct Beacon {
        uint8_t time[BEACON_TIMESTAMP_BYTES];
        uint8_t mac[BEACON_MAC_BYTES];
    };

    struct Beacon beacon;

    uint8_t keyStorage[PAL_AES_BYTES];
    cometos::ArbiterAction aesRequest;
    cometos::Callback<void()> encryptionCallback;
    void doEncryption();
    void encryptionReady(uint8_t* result);

    bool mic_updated;
    bool key_exists;
};

#endif
