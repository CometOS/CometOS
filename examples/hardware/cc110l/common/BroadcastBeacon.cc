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

#include "BroadcastBeacon.h"

using namespace cometos;

BroadcastBeacon::BroadcastBeacon() {
    aesRequest.setCallback(CALLBACK_MET(&BroadcastBeacon::doEncryption, *this));
    mic_updated = false;
    key_exists = false;
}

cometos_error_t BroadcastBeacon::setTime(BroadcastBeacon::timestamp_t timestamp)
{
    mic_updated = false;
    memcpy(beacon.time, ((uint8_t*)&timestamp), sizeof(beacon.time)); // copy from the least significant bytes (little endian)
    return COMETOS_SUCCESS;
}

BroadcastBeacon::timestamp_t BroadcastBeacon::getTime()
{
    BroadcastBeacon::timestamp_t timestamp = 0;
    memcpy((uint8_t*)&timestamp, beacon.time, sizeof(beacon.time)); // copy into the least significant bytes (little endian)
    return timestamp;
}

cometos_error_t BroadcastBeacon::setKey(const uint8_t *key)
{
    mic_updated = false;
    memcpy(keyStorage, key, PAL_AES_BYTES);
    key_exists = true;
    return COMETOS_SUCCESS;
}

cometos_error_t BroadcastBeacon::generateMIC(Callback<void()> callback)
{
    if(!key_exists) {
        return COMETOS_ERROR_FAIL; 
    }

    encryptionCallback = callback;

    // padding
    memset(fullMAC, 0, sizeof(fullMAC));
    memcpy(fullMAC,(uint8_t*)&beacon.time,sizeof(beacon.time));

    PalAES::getArbiter()->request(&aesRequest);
    return COMETOS_PENDING;
}

void BroadcastBeacon::doEncryption()
{
    ASSERT(PalAES::setKey(keyStorage) == COMETOS_SUCCESS);
    PalAES::encrypt(fullMAC, CALLBACK_MET(&BroadcastBeacon::encryptionReady,*this));
}

void BroadcastBeacon::encryptionReady(uint8_t*) {
    mic_updated = true;

    auto tmpcb = encryptionCallback;

    memcpy(beacon.mac,fullMAC,sizeof(beacon.mac)); // truncation

    PalAES::getArbiter()->release();

    if(tmpcb) {
        tmpcb();
    }
}

bool BroadcastBeacon::isMICMatching(BroadcastBeacon& other)
{
    for(uint8_t i = 0; i < sizeof(beacon.mac); i++) {
        if(other.beacon.mac[i] != beacon.mac[i]) {
                return false;
        }
    }

    return true;
}

cometos_error_t BroadcastBeacon::setData(uint8_t length, const uint8_t* data)
{
    if(length > sizeof(beacon)) {
        return COMETOS_ERROR_INVALID;
    }

    memcpy(&beacon,data,length);
    return COMETOS_SUCCESS;
}

const uint8_t* BroadcastBeacon::getData() const
{
    return (const uint8_t*)&beacon;
}

uint8_t BroadcastBeacon::getSize()
{
    return sizeof(beacon);
}

void BroadcastBeacon::print(OutputStream* outputStream) const
{
    if(outputStream == NULL) {
        outputStream = &getCout();
    }

    const uint8_t* data = getData();

    for (uint16_t i = 0; i < getSize(); i++) {
        if (i % 16 == 0) {
            (*outputStream) << endl << " ";
        }
        (*outputStream) << "0x";
        uint8_t d = data[i];
        if(d <= 0xF) {
            (*outputStream) << "0";
        }
        (*outputStream) << hex << (uint16_t)d << " ";
    }

    (*outputStream) << endl;
}

bool BroadcastBeacon::isMICUpdated()
{
    return mic_updated;
}

static const uint32_t DATARATE = 5768L;
#define SYNC CC110lApi::Sync::SYNC_16_16
#define PREAMBLE 4

uint16_t BroadcastBeacon::getIntervalMS() {
    constexpr uint16_t sync = (SYNC == CC110lApi::Sync::SYNC_30_32) ? 4 : 2;
    constexpr uint32_t bits = 8*(((uint16_t)sizeof(beacon))+PREAMBLE+sync);
    constexpr uint16_t interval = (10*bits*(uint32_t)1000)/DATARATE;
    return interval;
}

void BroadcastBeacon::setupTransceiver(CC110lApi& ccApi) {
    ccApi.setFrequency(869430);
    ccApi.setDataRate(DATARATE);
    ccApi.setChannelSpacing(60000);
    ccApi.setChannelBandwidth(60267);
    ccApi.setDeviation(DATARATE/2);

    ccApi.setSync(SYNC);
    ccApi.setPreamble(PREAMBLE);
    ccApi.setCoding(CC110lApi::Coding::UNCODED);
    ccApi.setFixedLength(sizeof(beacon));
    ccApi.setRXAttenuation(0);
}

uint16_t BroadcastBeacon::getPreambleSyncShift() {
    constexpr uint8_t sync = (SYNC == CC110lApi::Sync::SYNC_30_32) ? 4 : 2;
    constexpr uint16_t shift = ((sync+PREAMBLE)*8*(uint32_t)1000)/(DATARATE);
    return shift;
}

