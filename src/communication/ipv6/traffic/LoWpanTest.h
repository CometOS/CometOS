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

#ifndef LOWPANTEST_H_
#define LOWPANTEST_H_

#include "cometos.h"
#include "IPv6Request.h"

//#define printf(a, args...)

using namespace cometos;

class RunNumber: public Message {
public:
    RunNumber(uint8_t n) : Message(), run(n) { }
    uint8_t run;
};

class LoWpanTest: public Module {
public:
    LoWpanTest(const char * service_name = NULL);

    void initialize();

    void startTest(uint8_t testNum);

    void handleIPRequest(cometos_v6::IPv6Request *iprequest);

    void handleIPResponse(cometos_v6::IPv6Response *ipresponse);

    InputGate<cometos_v6::IPv6Request> fromIP;

    OutputGate<cometos_v6::IPv6Request>    toIP;

    void scheduleIPPacket(RunNumber *rn);
    void sendIPPacket(uint8_t testNum);

    //REMOVE THIS GEOGIN
#ifdef ENABLE_TESTING
    void scheduleAndPassOwnership(void* a){}
#endif
};

#endif /* LOWPANTEST_H_ */
