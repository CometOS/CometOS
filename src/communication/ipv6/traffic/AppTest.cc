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


#include "AppTest.h"

#define RUNS 10

#define NUM_FOLLOWING_HEADER 5

Define_Module(AppTest);

const uint16_t dstAddresses[RUNS][8] = {
        {0xFE80,0,0,0,0,0,0,1},
        {0xFE80,0,0,0,0,0,0,2},
        {0xFE80,0,0,0,0,0,0,3},
        {0xFE80,0,0,0,0,0,0,4},
        {0x64,0x29,0x30,0x31,0x32,0x33,0x0,0x5},
        {0x64,0x29,0x30,0x31,0x32,0x33,0x0,0x4},
        {0x64,0x29,0x30,0x31,0x32,0x33,0x0,0x3},
        {0x64,0x29,0x30,0x31,0x32,0x33,0x0,0x2},
        {0x64,0x29,0x30,0x31,0x32,0x33,0x0,0x1},
        {0x64,0x29,0x30,0x31,0x32,0x33,0x0,0x0},
};

const uint8_t  appMsg0[] = "00 Test Nachricht";
const uint8_t  appMsg1[] = "01 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...";
const uint8_t  appMsg2[] = "02 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert... Trallalla";
const uint8_t  appMsg3[] = "03 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...adfgaertkljsadöklg";
const uint8_t  appMsg4[] = "04 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert... asfklaföklaölkfawerkvcaö9ewikladjgi9"
        "dfga";
const uint8_t  appMsg5[] = "05 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfgläöekgölkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgölksdfj";
const uint8_t  appMsg6[] = "06 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfgläöekgölkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgölksdfjadfgadfgaergerg";
const uint8_t  appMsg7[] = "07 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfgläöekgölkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgölksdfjadfgafdgaergfvfnsdfgvkhkslehglkjhs";
const uint8_t  appMsg8[] = "08 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfgläöekgölkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgölksdfjdfgesjglksejglkjslgkjslkrjgklsrjglksrlktgjlkssklr";
const uint8_t  appMsg9[] = "09 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfgläöekgölkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgölksdfsdfgsdjfglkjerlkjglkjlkgjskglkjsdlkgjr9glsjglksdlkf"
        "dkfjölakejlksdjglkjgöljöeiorjgxkgjklsjrelkjgkjlksdfgkjlskjeigelgklsjj";

const uint8_t  *appMsg[RUNS] = {
        appMsg0, appMsg1, appMsg2, appMsg3, appMsg4, appMsg5, appMsg6, appMsg7, appMsg8, appMsg9
};

AppTest::AppTest(const char * service_name):
        cometos::Module(service_name)
{
}

AppTest::~AppTest()
{
    cancel(&r);
}

void AppTest::initialize() {
    bool isSender = false;
    CONFIG_NED(isSender);

    udp = (cometos_v6::UDPLayer*) getModule("udpLayer");
    icmp = (cometos_v6::ICMPv6*) getModule("icmPv6");

    port = udp->bind(this, (uint16_t)0xF0B0);

    LOG_DEBUG("Bound Port " << port);

    if (isSender) {
        r.run = 0;
        schedule(&r, &AppTest::scheduleMessage, 10);
    }
}

void AppTest::scheduleMessage(AppRunNumber *rn) {

    sendMessage(rn->run);

    rn->run++;

    if (rn->run == RUNS) {
        rn->run = 0;
    }
    if (rn->run != 0)
    schedule(rn, &AppTest::scheduleMessage, 1500);
}

void AppTest::startTest(uint8_t testNum) {
    sendMessage(testNum);
}

void AppTest::sendMessage(uint8_t testNum) {
    uint16_t len;
    for (len = 0; appMsg[testNum][len] != 0; len++);

    IPv6Address dst = dstAddresses[testNum];

    LOG_DEBUG("Send UDP Message to [" << dst.str() << "]:" << port);
    udp->sendMessage(dst, port, port, appMsg[testNum], len, NULL);

    LOG_DEBUG("Send Echo Request Sequence " << (int)testNum);
    icmp->sendEchoBI(dst, testNum, NULL, this, &AppTest::receiveEchoReply);
}

void AppTest::receiveEchoReply(uint16_t seqNum, bool success) {
    if (success) {
        LOG_DEBUG("Echo Reply returned. Sequence " << seqNum);
    } else {
        LOG_DEBUG("Echo Sequence " << seqNum << " timed out!");
    }
}

void AppTest::udpPacketReceived(const IPv6Address& src,
        uint16_t srcPort,
        uint16_t dstPort,
        const uint8_t* data,
        uint16_t length)
{
    bool allright = true;
    uint8_t run = (data[0] - 0x30) * 10 + (data[1] - 0x30);

    uint16_t len;
    for (len = 0; appMsg[run][len] != 0; len++);

    if (length != len) {
        LOG_DEBUG("AppTest: Message Length wrong!");
        printf("AppTest: Message Length wrong! %d != %d\n",
                length,
                len);
        allright = false;
    }

    if (allright) {
        LOG_DEBUG("Success! RUN " << (int)run);
        printf("Success!\n");
    } else {
        LOG_DEBUG("Failure! RUN " << (int)run);
        printf("Failure!\n");
    }

}
