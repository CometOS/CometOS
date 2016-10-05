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

#include "LoWpanTest.h"
#include "UDPPacket.h"
#include "IPv6DestinationOptionsHeader.h"
#include "IPv6FragmentHeader.h"
#include "IPv6HopByHopOptionsHeader.h"
#include "IPv6RoutingHeader.h"
#include "ICMPv6Message.h"

#define RUNS 10

#define NUM_FOLLOWING_HEADER 5

Define_Module(LoWpanTest);

const uint16_t srcAddresses[RUNS][8] = {
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1},
        {0xFE80,0,0,0,0,0,0,1},
        {0xFE80,0,1,0,1,0,1,2},
        {0x2001,0xdb8,0,0x8d3,0,0x8a2e,0x70,0x7344},
        {0xfc00,1,2,3,4,5,6,7},
        {0x2000,8,9,10,11,12,13,14},
        {0x2002,15,16,17,18,19,20,21},
        {0x3ffe,22,23,24,25,26,27,28},
        {0x64,29,30,31,32,33,34,35},
};

const uint16_t dstAddresses[RUNS][8] = {
        {0xFF02,0,0,0,0,0,0,2},
        {0xFE80,0,1,0,1,0,1,1},
        {0xFF01,0,0,0,0,0,0,1},
        {0xFF05,0,0,0,0,0,0,2},
        {0xFE80,0,0,0,0,0x8a2e,0x70,0x7344},
        {0xfc00,1,2,3,4,5,6,7},
        {0x2000,8,9,10,11,12,13,14},
        {0x2002,15,16,17,18,19,20,21},
        {0x3ffe,22,23,24,25,26,27,28},
        {0x64,29,30,31,32,33,34,35},
};


const uint8_t hopLimits[RUNS] = {
        1, 64, 255, 2, 3, 4, 5, 6, 7, 8
};

const uint8_t trafficClasses[RUNS] = {
        0, 1, 128, 0, 1, 128, 0, 1, 129, 0
};

const uint32_t flowLabels[RUNS] = {
        0, 0, 0, 1, 1, 1, 2, 2, 2, 3
};

const uint8_t  msg0[] = "00 Test Nachricht";
const uint8_t  msg1[] = "01 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...";
const uint8_t  msg2[] = "02 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert... Trallalla";
const uint8_t  msg3[] = "03 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...adfgaertkljsadäklg";
const uint8_t  msg4[] = "04 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert... asfklafäklaälkfawerkvcaä9ewikladjgi9"
        "dfga";
const uint8_t  msg5[] = "05 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfglääekgälkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgälksdfj";
const uint8_t  msg6[] = "06 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfglääekgälkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgälksdfjadfgadfgaergerg";
const uint8_t  msg7[] = "07 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfglääekgälkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgälksdfjadfgafdgaergfvfnsdfgvkhkslehglkjhs";
const uint8_t  msg8[] = "08 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfglääekgälkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgälksdfjdfgesjglksejglkjslgkjslkrjgklsrjglksrlktgjlkssklr";
const uint8_t  msg9[] = "09 Test Nachricht\nDiese Nachricht ist nun etwas "
        "länger, mal sehen was passiert...fdgsadfglääekgälkäeglksfdgäoegklsdkfg"
        "oglkgslgdgsdgälksdfsdfgsdjfglkjerlkjglkjlkgjskglkjsdlkgjr9glsjglksdlkf"
        "dkfjälakejlksdjglkjgäljäeiorjgxkgjklsjrelkjgkjlksdfgkjlskjeigelgklsjj";

const uint8_t  *msg[RUNS] = {
        msg0, msg1, msg2, msg3, msg4, msg5, msg6, msg7, msg8, msg9
};

using namespace cometos_v6;

LoWpanTest::LoWpanTest(const char * service_name) :
        cometos::Module(service_name),
        fromIP(this, &LoWpanTest::handleIPRequest, "fromIP"),
        toIP(this, "toIP")
{}

void LoWpanTest::initialize() {
    bool isSender = false;
    CONFIG_NED(isSender);

    if (isSender) {
        RunNumber* r = new RunNumber(9);
        schedule(r, &LoWpanTest::scheduleIPPacket, 10);
    }
}

void LoWpanTest::scheduleIPPacket(RunNumber *rn) {

    sendIPPacket(rn->run);

    rn->run++;

    if (rn->run < RUNS) {
        schedule(rn, &LoWpanTest::scheduleIPPacket, 1500);
    } else {
        delete rn;
    }
}

void LoWpanTest::startTest(uint8_t testNum) {
    sendIPPacket(testNum);
}

void LoWpanTest::sendIPPacket(uint8_t testNum) {
    //UDPPacket* udp = new UDPPacket();
    ICMPv6Message* icmp = new ICMPv6Message(128, 0);
    node_t initialDest;
    CONFIG_NED(initialDest);
    uint16_t   src[4] = {0,0,0,getId()};
    uint16_t   dst[4] = {0,0,0,initialDest}; // for now, use BROADCAST on MAC layer
    IPv6Datagram* testDatagram = new IPv6Datagram();

    testDatagram->src = srcAddresses[testNum];
    testDatagram->dst = dstAddresses[testNum];

    testDatagram->setHopLimit(hopLimits[testNum]);
    testDatagram->setTrafficClass(trafficClasses[testNum]);
    testDatagram->setFlowLabel(flowLabels[testNum]);

    uint16_t len;
    for (len = 0; msg[testNum][len] != 0; len++);
    icmp->setData(msg[testNum], len);
    icmp->setChecksum(0x1234);

    testDatagram->addHeader(new IPv6HopByHopOptionsHeader());
    testDatagram->addHeader(new IPv6RoutingHeader());
    testDatagram->addHeader(new IPv6FragmentHeader());
    testDatagram->addHeader(new IPv6DestinationOptionsHeader());

    testDatagram->addHeader(icmp);

    LOG_DEBUG("Sending ICMP Packet with data size: " << icmp->getUpperLayerPayloadLength());
    uint16_t offset = intrand(200)+100;
    printf("LoWPANTest: send ip packet in %d ms; src[3]=%x|dst[3]=%x\n",
            offset, src[3], dst[3]);
    //REMOVE THIS GEOGIN
#ifndef ENABLE_TESTING
    toIP.send(new cometos_v6::IPv6Request(
            src,
            dst,
            testDatagram,
            createCallback(&LoWpanTest::handleIPResponse)), offset);
#endif

}

void LoWpanTest::handleIPRequest(cometos_v6::IPv6Request *iprequest) {

    iprequest->response(new cometos_v6::IPv6Response(iprequest, IPv6Response::IPV6_RC_SUCCESS));

    uint8_t numHeaders = iprequest->data.datagram->getNumNextHeaders();
    bool allright = true;

    if (numHeaders != NUM_FOLLOWING_HEADER) {
        allright = false;
        printf("LoWPANTest: Number of Following Headers: %d\n",numHeaders);
        for (uint8_t i = 0; i < numHeaders; i++) {
            printf("LoWPANTest: Header %d of Type: %d\n",
                    i, iprequest->data.datagram->getNextHeader(i)->getHeaderType());
        }
    }

    if (iprequest->data.datagram->getNextHeader(numHeaders-1)->getHeaderType() == 17) {

        LOG_DEBUG("UDP Src Port: " <<
                ((UDPPacket*)iprequest->data.datagram->getNextHeader(numHeaders-1))->getSrcPort());
        LOG_DEBUG("UDP Dst Port: " <<
                ((UDPPacket*)iprequest->data.datagram->getNextHeader(numHeaders-1))->getDestPort());

    }
    const uint8_t* data = iprequest->data.datagram->getNextHeader(numHeaders-1)->getData();

    uint8_t run = (data[0] - 0x30) * 10 + (data[1] - 0x30);

    uint16_t len;
    for (len = 0; msg[run][len] != 0; len++);

    printf("LoWPANTest: run %d; rcvd ip packet; src[3]=%x|dst[3]=%x\n",
            run, iprequest->data.srcMacAddress.a4(), iprequest->data.dstMacAddress.a4());

    if (iprequest->data.datagram->getNextHeader(numHeaders-1)->getUpperLayerPayloadLength() != len) {
        printf("LoWPANTest: Message Length wrong! %d != %d\n",
                iprequest->data.datagram->getNextHeader(numHeaders-1)->getUpperLayerPayloadLength(),
                len);
        allright = false;
    }

    if (iprequest->data.datagram->src != srcAddresses[run]) {
        printf("LoWPANTest: Src Adress invalid!\n");
        printf("LoWPANTest: Src Address: %X:%X:%X:%X:%X:%X:%X:%X\n",
                iprequest->data.datagram->src.getAddressPart(0),
                iprequest->data.datagram->src.getAddressPart(1),
                iprequest->data.datagram->src.getAddressPart(2),
                iprequest->data.datagram->src.getAddressPart(3),
                iprequest->data.datagram->src.getAddressPart(4),
                iprequest->data.datagram->src.getAddressPart(5),
                iprequest->data.datagram->src.getAddressPart(6),
                iprequest->data.datagram->src.getAddressPart(7));
        allright = false;
    }
    if (iprequest->data.datagram->dst != dstAddresses[run]) {
        printf("LoWPANTest: Dst Adress invalid!\n");
        printf("Dst Address: %X:%X:%X:%X:%X:%X:%X:%X\n",
                iprequest->data.datagram->dst.getAddressPart(0),
                iprequest->data.datagram->dst.getAddressPart(1),
                iprequest->data.datagram->dst.getAddressPart(2),
                iprequest->data.datagram->dst.getAddressPart(3),
                iprequest->data.datagram->dst.getAddressPart(4),
                iprequest->data.datagram->dst.getAddressPart(5),
                iprequest->data.datagram->dst.getAddressPart(6),
                iprequest->data.datagram->dst.getAddressPart(7));
        allright = false;
    }
    if (iprequest->data.datagram->getFlowLabel() != flowLabels[run]) {
        printf("FlowLabel invalid: %d != %d\n",
                iprequest->data.datagram->getFlowLabel(),
                flowLabels[run]);
        allright = false;
    }
    if (iprequest->data.datagram->getTrafficClass() != trafficClasses[run]) {
        printf("TrafficClass invalid: %d != %d\n",
                iprequest->data.datagram->getTrafficClass(),
                trafficClasses[run]);
        allright = false;
    }

    if (allright) {
        printf("Success!\n");
    } else {
        printf("Failure!\n");
    }

    delete iprequest;

}

void LoWpanTest::handleIPResponse(cometos_v6::IPv6Response *ipresponse) {
    if (ipresponse->success) {
        LOG_DEBUG("Received positive Response");
    } else {
        LOG_DEBUG("Received negative Response");
    }
    delete ipresponse;
}
