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

#include "BeaconApp.h"
#include "cometos.h"
#include "palPin.h"
#include "NetworkTime.h"
#include "palGPIOPin.h"
#include "CC110lApi.h"
#include "BroadcastBeacon.h"
#include "palLocalTime.h"

using namespace cometos;

DEFINE_OUTPUT_PIN(fault,PORTB,4);

uint16_t beaconsLost = 0;
static const uint16_t beaconsLostResync = 4*5;
static const uint8_t missingBeaconsUntilFault = 4;
static const uint8_t FAULT_TOLERANCE = 2;
void fault();
SimpleTask faultTask(fault);

void fault() {
    fault_on();
}

void setup();

GPIOPin<IOPort::E,7> gdo0pin;
GPIOPin<IOPort::E,3> chipSelect;
GPIOPin<IOPort::B,3> miso;
static cometos::CC110lApi longrange(&gdo0pin,&chipSelect,&miso,CALLBACK_FUN(setup));

DEFINE_OUTPUT_PIN(pe6, PORTE, 6);

// last beacon
static BroadcastBeacon::timestamp_t lastTimestamp = 0;

static time_ms_t localReceptionTime = 0;
static int16_t esum = 0;
static int32_t Kp = 400;
static int32_t Ki = 50;
void packetStart() {
    localReceptionTime = palLocalTime_get();
    uint8_t tval = TCNT2;

    // only if no beacon missed
    int32_t interval = BroadcastBeacon::getIntervalMS();
    int64_t e = tval-(192);
    {
        int16_t curr = XOSC_CTRL & (0xF);
        XOSC_CTRL &= ~(0xF);

        esum = esum + e;

        // anti windup
        int16_t windupBound = 100;
        if(esum < -windupBound) {
            esum = -windupBound;
        }
        if(esum > windupBound) {
            esum = windupBound;
        }


        int16_t y = (Kp * e)/1000 + (Ki * interval * esum)/(100000);
        curr = y;

        if(curr < 0) {
            curr = 0;
        }
        else if(curr > 0xF) {
            curr = 0xF;
        }
        XOSC_CTRL |= curr;
    }
}

static const int16_t eps = 2;
uint8_t lastBeaconChannel = 0;
int16_t lastBeaconRSSI = INT16_MIN;
bool beaconFound = false;
bool synchronizationActive = true;
bool synchronized = false;

void receiveLRBeacon(uint8_t length, uint8_t* data, int16_t rssi, uint8_t crc_ok);

void initSynchronization();

void initNormalReception();
SimpleTask initReceptionTask(initNormalReception);
void initNormalReception() {
    time_ms_t now = NetworkTime::get();
    uint16_t interval = BroadcastBeacon::getIntervalMS();
    uint16_t intervalsSinceBeacon = (now-lastTimestamp+interval/2)/interval; // rounding up

    time_ms_t wakeupTime = lastTimestamp+(intervalsSinceBeacon+1)*(time_ms_t)interval-eps; // 2* since the lastTimestamp is still the one of the previous

    time_ms_t sleepDuration = 0;
    if(wakeupTime > now) {
        sleepDuration = wakeupTime-now;
    }
    getScheduler().add(initReceptionTask,sleepDuration);

    // tune to new channel (should be done before entering RX mode to start the calibration on the correct frequency)
    uint8_t expectedChannel = (lastBeaconChannel+intervalsSinceBeacon)%NUM_CHANNELS;
    longrange.setChannel(expectedChannel);

    // switch to RX mode
    if(!beaconFound && synchronized) {
        cometos::getCout() << "{n}" << cometos::endl;
        beaconsLost++;
        if(beaconsLost > beaconsLostResync) {
            beaconsLost = 0;
            getScheduler().remove(initReceptionTask);
            initSynchronization();
        }
    }
    beaconFound = false;
    longrange.receivePacket(CALLBACK_FUN(receiveLRBeacon));
}

void endSynchronization();
SimpleTask endSynchronizationTask(endSynchronization);
void initSynchronization() {
    synchronizationActive = true;
    synchronized = false;
    lastBeaconRSSI = INT16_MIN;
    getCout() << "Start Synchronization" << endl;
    longrange.setChannel(0);
    longrange.receivePacket(CALLBACK_FUN(receiveLRBeacon));
    getScheduler().add(endSynchronizationTask, (NUM_CHANNELS+1)*BroadcastBeacon::getIntervalMS());
}

void endSynchronization() {
    synchronizationActive = false;
    getCout() << "End Synchronization" << endl;
    if(lastBeaconRSSI <= INT16_MIN) {
        initSynchronization();
    }
    else {
        initNormalReception();
    }
}

static bool busy = false;
static int16_t currentRSSI;
void micCalculated();

BroadcastBeacon receiveBeacon;
BroadcastBeacon validationBeacon;

void receiveLRBeacon(uint8_t length, uint8_t* data, int16_t rssi, uint8_t crc_ok) {
    if(synchronizationActive) { // in normal mode this is done by the wakeup
        longrange.receivePacket(CALLBACK_FUN(receiveLRBeacon));
    }

    if(length != BroadcastBeacon::getSize()) {
        getCout() << "wrong len " << dec << (uint16_t)length << endl;
        return;
    }

    palExec_atomicBegin();
    if(busy) {
        cometos::getCout() << "Beacon reception busy" << cometos::endl;
        palExec_atomicEnd();
        return;
    }
    busy = true;
    palExec_atomicEnd();

    static int cnt = 0;
    cnt++;
    receiveBeacon.setData(length,data);
    currentRSSI = rssi;

    if(lastTimestamp >= receiveBeacon.getTime()) {
        cometos::getCout() << "Timestamp invalid -> ignored" << cometos::endl;
        busy = false;
        return;
    }
    else {
        validationBeacon.setTime(receiveBeacon.getTime());

#ifndef BEACON_AES_KEY
#error "No BEACON_AES_KEY defined!"
#endif
#define VAL_STR(x) STR(x)
#define STR(x) #x
        validationBeacon.setKey((const uint8_t*)VAL_STR(BEACON_AES_KEY));
        validationBeacon.generateMIC(CALLBACK_FUN(micCalculated));
    }
}

void printRSSI() {
    cometos::getCout() << dec << ((int16_t)(currentRSSI/10)) << "." << ((int16_t)((currentRSSI>0?currentRSSI:-currentRSSI)%10));
}

void micCalculated() {
    if(!receiveBeacon.isMICMatching(validationBeacon)) {
        if(!synchronizationActive) {
            cometos::getCout() << "{i,";
            printRSSI();
            cometos::getCout() << "}" << cometos::endl;
            beaconFound = true;

            beaconsLost++;
            if(beaconsLost > beaconsLostResync) {
                beaconsLost = 0;
                getScheduler().remove(initReceptionTask);
                initSynchronization();
            }
        }
        else {
            cometos::getCout() << "." << cometos::endl;
        }
        busy = false;
        return;
    }
    else {
        if(synchronizationActive) {
            if(currentRSSI < lastBeaconRSSI) {
                busy = false;
                return;
            }
        }

        lastTimestamp = validationBeacon.getTime();
        lastBeaconChannel = longrange.getChannel();
        lastBeaconRSSI = currentRSSI;

        if(!synchronizationActive) {
            synchronized = true;

            // beacon valid! shift fault task
            cometos::getScheduler().replace(faultTask,missingBeaconsUntilFault*BroadcastBeacon::getIntervalMS()+FAULT_TOLERANCE);
            fault_off();
            beaconsLost = 0;

            cometos::getCout() << "{y,";
            printRSSI();
            cometos::getCout() << "}" << cometos::endl;

            beaconFound = true;
        }
        NetworkTime::setOffset(validationBeacon.getTime()-localReceptionTime + BroadcastBeacon::getPreambleSyncShift());

        busy = false;
        return;
    }

}

void setup() {
    BroadcastBeacon::setupTransceiver(longrange);
}

void init();
SimpleTask initTask(init);
void init() {
    fault_init();
    fault_on();
    miso.pullup(true);

    XOSC_CTRL &= ~(0xF);
    //XOSC_CTRL |= 0x0; // FAKE TO CHECK VALIDITY OF SYNCHRONIZATION ALGORITHM

    longrange.setup();
    longrange.setPacketStartCallback(CALLBACK_FUN(packetStart));
    initSynchronization();
}

void BeaconApp::initialize() {
    cometos::getScheduler().add(initTask,2000);
}
