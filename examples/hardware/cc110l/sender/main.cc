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

/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "palLed.h"
#include "Task.h"
#include "OutputStream.h"
#include "palExecUtils.h"
#include "palGPIOPin.h"
#include "BroadcastBeacon.h"
#include "palLocalTime.h"

#include "palPin.h"

#include "CC110lApi.h"
#include "NetworkTime.h"

using namespace cometos;

#ifdef BOARD_AutoRNode
GPIOPin<IOPort::E,7> gdo0pin;
GPIOPin<IOPort::E,3> chipSelect;
GPIOPin<IOPort::B,3> miso;
#else
#ifdef BOARD_lorasender
GPIOPin<IOPort::D,2> gdo0pin;
GPIOPin<IOPort::B,2> chipSelect;
GPIOPin<IOPort::B,4> miso;
#endif
#endif

void setup();

CC110lApi sender(&gdo0pin,&chipSelect,&miso,CALLBACK_FUN(setup));

BroadcastBeacon bb;

void initBeacon();
void sendBeacon();
SimpleTask initBeaconTask(initBeacon);
SimpleTask sendBeaconTask(sendBeacon);

static const uint16_t micDelay = 10;
static uint8_t channel = 0;

DEFINE_OUTPUT_PIN(pc3, PORTC, 3);
DEFINE_OUTPUT_PIN(pc4, PORTC, 4);

void recv(uint8_t len, uint8_t* pData) {
	// unused
}

void micGenerated() {
    sender.receivePacket(CALLBACK_FUN(recv));
    sender.preparePacket(bb.getSize(),bb.getData());
}

static int16_t beaconInterval;

void initBeacon() {
    time_ms_t time = palLocalTime_get();
    time_ms_t timeStamp = ((time/beaconInterval)+1)*beaconInterval;
    
    int16_t delay = (beaconInterval-(time+micDelay)%beaconInterval);
    if(delay < beaconInterval/4) {
    	cometos::getScheduler().add(initBeaconTask, delay+beaconInterval);
    	getCout() << "Beacon missed" << endl;
    }
    else {
    	cometos::getScheduler().add(initBeaconTask, delay);
    }

    pc4_on();
    palLed_on(1 << 1);

    bb.setTime(timeStamp);
    bb.generateMIC(CALLBACK_FUN(micGenerated));

    channel = (channel+1)%NUM_CHANNELS;
    sender.setChannel(channel);
}

void printTime();
SimpleTask printTimeTask(printTime);
static const timeOffset_t printTimeInterval = 1123;

inline void sendBeacon() {
    if(bb.isMICUpdated()) {
        sender.send();
        pc4_off();
    }
    else {
        getCout() << "MIC not yet calculated" << endl;
    }
}

ISR( TIMER2_COMPB_vect ) {
    time_ms_t time = NetworkTime::get();
    if(time % beaconInterval == 0) {
        sendBeacon();
    }
}

void printTime() {
    time_ms_t time = NetworkTime::get();
    getScheduler().add(printTimeTask, printTimeInterval - time%printTimeInterval); // print on full ten seconds

    if(time % printTimeInterval == 0) {
        if((time / printTimeInterval) % 2 == 0) {
            pc3_on();
        }
        else {
            pc3_off();
        }
    }
}

void setup() {
    sender.enableAmp();
    sender.setUnamplifiedOutputPower(0);
    BroadcastBeacon::setupTransceiver(sender);
}

int main() {
    palLed_init();
    cometos::initialize();

    cometos::getCout() << "Start Longrange Application\n";

    cometos::getCout() << (int)palExec_getResetReason() << cometos::endl;
    cometos::getCout() << cometos::hex << palExec_getPcAtReset() << cometos::endl;

    cometos::getScheduler().add(initBeaconTask, 4000);

#ifndef BEACON_AES_KEY
#error "No BEACON_AES_KEY defined!"
#endif
#define VAL_STR(x) STR(x)
#define STR(x) #x
    bb.setKey((const uint8_t*)VAL_STR(BEACON_AES_KEY));

    sender.setup();
    pc3_init();
    pc4_init();

    beaconInterval = BroadcastBeacon::getIntervalMS(); 

    cometos::getScheduler().add(printTimeTask,0);

    TIMSK2 |= (1 << OCIE2B);
    OCR2B = (2*(uint16_t)OCR2A)/10;

    cometos::run();
    return 0;
}
