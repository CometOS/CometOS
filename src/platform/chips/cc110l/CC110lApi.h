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

#ifndef CC110LAPI_H_
#define CC110LAPI_H_

/******************************************************************************
 * INCLUDES
 */

#include <stdint.h>
#include "cc_common.h"
#include "CC11xlRegisters.h"
#include "CC1190Api.h"
#include "CCSpi.h"

#define LINK_1_WAY 0
#define LINK_2_WAY 1

#define MASTER_DEVICE 1
#define SLAVE_DEVICE 0

#define PER_DEVICE_LINKED       1
#define PER_DEVICE_NOT_LINKED   0

#define PER_DEVICE_LINK_BYPASS  2

#define PER_SETTINGS_PACKET_LEN            9
#define PER_MAX_DATA                      64

#define SYNC_FOUND              ((PORTB & PB1) >= 1) ? 1 : 0 /* PIN PB1 */
#define PACKET_RECEIVED         ((PORTB & PB0) >= 1) ? 1 : 0 /* PIN PB0 */

/* Chip type constants */
#define CHIP_TYPE_CC1101                0x1101

#define CHIP_TYPE_CC110L                0x0110
#define CHIP_TYPE_CC113L                0x0113
#define CHIP_TYPE_CC115L                0x0115

#define CHIP_TYPE_CC2500                0x2500
#define CHIP_TYPE_NONE                  0x0000

namespace cometos {

class CC110lApi: public CCSpi
{
public:
    typedef struct
    {
        uint16_t deviceName;
        uint16_t id;
        uint8_t  ver;
    }radioChipType_t;

    enum class Modulation : uint8_t {
        FSK2,
        GFSK,
        OOK
    };

    enum class Coding : uint8_t {
        UNCODED,
        MANCHESTER
    };

    enum class Sync : uint8_t {
        NO_SYNC = 0,
        SYNC_15_16 = 1,
        SYNC_16_16 = 2,
        SYNC_30_32 = 3,
        NO_SYNC_CS = 4,
        SYNC_15_16_CS = 5,
        SYNC_16_16_CS = 6,
        SYNC_30_32_CS = 7
    };

    enum class CCAMode : uint8_t {
        ALWAYS,
        RSSI,
        RECEIVING,
        RSSI_RECEIVING
    };

    typedef Callback<void(uint8_t length, uint8_t* data, int16_t rssi, uint8_t crc_ok)> dataAvailableCallback_t;
    typedef Callback<void()> packetStartCallback_t;
    typedef Callback<void(bool rx)> packetEndCallback_t;
    typedef Callback<void(uint8_t length, uint8_t *pData)> receive_callback_t;
    typedef Callback<void(uint8_t length, uint8_t *pData, int16_t rssi, uint8_t crc_ok)> receive_callback_ext_t;

    /**************************************************************************
    * Public functions
    */

    CC110lApi(IGPIOPin* gdo0pin, IGPIOPin* chipSelectPin, IGPIOPin* misoPin, Callback<void()> setupRoutine = EMPTY_CALLBACK());

    /* Will populate the radioChipType struct when called */
    uint8_t detectChipType(radioChipType_t *pRadioChipType);

    void regConfig();

    cometos_error_t send();

    void setup();
    void receivePacket(receive_callback_t receiveCallback);
    void receivePacket(receive_callback_ext_t receiveCallback);
    void enableAmp();

    cometos_error_t preparePacket(uint8_t length, const uint8_t *pData, Callback<void()> txReady = EMPTY_CALLBACK());

    cometos_error_t send(uint8_t length, const uint8_t *pData, Callback<void()> txReady = EMPTY_CALLBACK());

    void receivePacket(void);
    void receiveContinuously(void);
    void setDataAvailableCallback(dataAvailableCallback_t callback);
    void setPacketEndCallback(packetEndCallback_t callback);
    void setPacketStartCallback(packetStartCallback_t callback);
    void stateChangeConnect(Callback<void(ccStatus_t)> callback);

    void readPacket();
    void readoutFIFOWhileContinous();

    uint8_t getMode();

    int8_t read8BitRssi(void);

    void enterRx(void);
    void enterIdle(void);
    void enterSleep(void);

    void dumpSettings(void);

    int8_t getMinUnamplifiedOutputPower();
    int8_t getMaxUnamplifiedOutputPower();
    cometos_error_t setUnamplifiedOutputPower(int8_t powerdBm);
    cometos_error_t setDataRate(uint32_t datarateBaud);
    cometos_error_t setModulation(Modulation modulation);
    cometos_error_t setCoding(Coding coding);
    cometos_error_t setSync(Sync sync);
    cometos_error_t setCRC(bool enable);
    cometos_error_t setFixedLength(uint8_t length);
    cometos_error_t setFrequency(uint32_t freqKHZ);
    cometos_error_t setFrequencyOffset(int8_t offset);
    cometos_error_t setPreamble(uint8_t bytes);
    cometos_error_t setDeviation(uint32_t deviationHertz);
    cometos_error_t setChannelBandwidth(uint32_t bandwidthHertz);
    cometos_error_t setCCAMode(CCAMode ccaMode);
    cometos_error_t setChannelSpacing(uint32_t channelSpacingHertz);
    cometos_error_t setChannel(uint8_t channel);
    cometos_error_t setRXAttenuation(uint8_t attenuation);
    uint8_t getChannel();

    void sendStatic();
    void printLastReceptionParameters();

    CC1190Api& getCC1190();

private:
    /**************************************************************************
     * Private functions
     */

    CC110lApi(CC110lApi const&);            // Don't Implement
    void operator=(CC110lApi const&);       // Don't implement

    void rxIdle(void);
    void idleRx(void);
    int8_t convert8BitRssi(uint8_t rawRssi);

    void enableInt(void);
    void disableInt(void);

    void changeState(ccStatus_t newState);
    ccStatus_t getState();

    void interpreteRxData(uint8_t length, uint8_t *pData, int16_t rssi, uint8_t crc_ok);
    void printChipType(void);
    void callStateChangeCallback(ccStatus_t newState);

    void pinInterrupt(void);

    /**************************************************************************
     * Private members
     */
    volatile ccStatus_t rfState;

    BoundedTask<CC110lApi, &CC110lApi::readPacket> readTask;
    BoundedTask<CC110lApi, &CC110lApi::readoutFIFOWhileContinous> readoutFIFOTask;

    dataAvailableCallback_t dataAvailableCallback;
    packetStartCallback_t packetStartCallback;
    packetEndCallback_t packetEndCallback;
    const uint8_t cc11xLRssiOffset = 74;
    bool packetPending;

    bool packetMode;

    Callback<void(ccStatus_t)> stateChangeCallback;
    Callback<void()> setupRoutine;
    IGPIOPin* gdo0pin;

    uint8_t channel;
    bool hasFixedLength;
    uint8_t fixedLength;

    int16_t lastRSSI;
    int16_t lastFREQEST;

    CC1190Api cc1190;
    receive_callback_t receiveCallback;
    receive_callback_ext_t receiveCallbackExt;
    Callback<void()> txReadyCallback;
    ccStatus_t previousState;
};

} /* namespace cometos */

#endif /* CC110LAPI_H_ */
