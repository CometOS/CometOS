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
 * @author Martin Ringwelski
 */

#ifdef OMNETPP
#include <ctime>
#include <math.h>
#endif
#ifndef OMNETPP
#include "math.h"
#endif

#include "palLed.h"

#include "BorderRouterTest.h"
#include "palLocalTime.h"
#include "palId.h"


Define_Module(BorderRouterTest);

BorderRouterTest::BorderRouterTest(const char * service_name) :
            cometos::LongScheduleModule(service_name),
            udp(NULL),
            port(0)
{
}


void BorderRouterTest::initialize() {
    cometos::LongScheduleModule::initialize();

    LOG_DEBUG("init");

    udp = (cometos_v6::UDPLayer*) getModule(UDP_MODULE_NAME);
    const int LISTENINGPORT = 22222;
    port = udp->bind(this, LISTENINGPORT);
    palLed_toggle(4);

}

void BorderRouterTest::finish() {
}

#if defined BOARD_devboard
#include <avr/io.h>
// Get temperature like datasheet says:
// http://www.atmel.com/Images/doc8266.pdf on page 429-430


short getDeciTemp() {

    //     read temperature of Atmel ATmega128RFA1 in deci to get a better resolution
    ADCSRC = 10<<ADSUT0; // set start-up time
    ADCSRB = 1<<MUX5; // set MUX5 first
    ADMUX = (3<<REFS0) + (9<<MUX0);
    // switch ADC on, set prescaler, start conversion
    ADCSRA = (1<<ADEN) + (1<<ADSC) + (4<<ADPS0);
    do {} while( (ADCSRA & (1<<ADSC))); // wait for conversion end
    ADCSRA = 0;

    // Convert uint16_t adc value to real temp as in data sheet described:

    // ADC needs to be scaled to 1.13 * ADC / 273
    short temp = (short) (ADC * 11.3 - 2728);

    return temp;
}

short getTemp() {
    return getDeciTemp() / 10;
}

short getDeciTempAvg(int n) {
    short tempAvg = 0;
    for(int i=0;i<n;i++) {
        tempAvg += getDeciTemp();
    }

    return tempAvg / n;

}

#endif

void BorderRouterTest::udpPacketReceived(const IPv6Address& src,
        uint16_t srcPort,
        uint16_t dstPort,
        const uint8_t* data,
        uint16_t length)
{

    palLed_toggle(2);

    const uint16_t maxLen = 10;
    static uint8_t infodata[maxLen] = {0};

    #if defined BOARD_devboard

        short temp = getDeciTempAvg(5);

        // temp is 10 times real temp which should be between -40°C and 125°C
        if(temp>-400 && temp < 1250) {
            sprintf((char*) infodata,"%d.%d C",temp/10,temp%10);
        }
        else {
            // Out of temperature range for uc!
            sprintf((char*) infodata,"OOR ERROR");
        }

    #else
        sprintf((char*) infodata, "xx.x C"); // only for testing in simulator
    #endif

    // srcPort = new DstPort
    uint16_t newdstPort = srcPort;
    uint16_t newsrcPort = 0;
    udp->sendMessage(src, newsrcPort, newdstPort, infodata, sizeof(infodata), NULL);

}
