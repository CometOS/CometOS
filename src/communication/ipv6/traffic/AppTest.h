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

#ifndef APPTEST_H_
#define APPTEST_H_

#include "cometos.h"
#include "IPv6Request.h"
#include "UDPLayer.h"
#include "ICMPv6.h"


class AppRunNumber: public cometos::Message {
public:
    AppRunNumber(uint8_t n = 0) : cometos::Message(), run(n) { }
    uint8_t run;
};

class AppTest: public cometos::Module, public cometos_v6::UDPListener {
public:
    AppTest(const char * service_name = NULL);
    ~AppTest();

    void initialize();

    void startTest(uint8_t testNum);

    void scheduleMessage(AppRunNumber *rn);
    void sendMessage(uint8_t testNum);

    virtual void udpPacketReceived(const IPv6Address& src,
                uint16_t srcPort,
                uint16_t dstPort,
                const uint8_t* data,
                uint16_t length);

    void receiveEchoReply(uint16_t seqNum, bool success);

protected:
    cometos_v6::UDPLayer*   udp;
    cometos_v6::ICMPv6*     icmp;

    AppRunNumber r;
    uint16_t port;

};

#endif /* APPTEST_H_ */
