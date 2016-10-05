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
 * @author: Marvin Burmeister, Martin Ringwelski
 */

#ifndef COAPTEST_H_
#define COAPTEST_H_

#include "cometos.h"
#include "CoAPLayer.h"
#include "palId.h"
#include "palLocalTime.h"

#define NUM_MAX_NODES 32

class CoAPTest: public cometos::LongScheduleModule, cometos_v6::CoAPListener {
public:
    CoAPTest(const char * service_name = NULL);
    ~CoAPTest() {
        for (uint16_t i = 0; i < numRes; i++) {
            delete[] resPaths[i];
            delete[] resTypes[i];
        }
        delete[] resPaths;
        delete[] resTypes;
    }
    void initialize();
    void sendTimerFired(cometos::LongScheduleMsg * msg);
    void finish();

    void incommingCoAPHandler(const cometos_v6::CoAPMessage* incoming, const bool state);

private:
    cometos_v6::CoAPLayer*  coapLayer;

    uint16_t receiver;
    bool sender;

    uint16_t port;
    uint16_t counter;
    uint16_t counter2;

    //Client only
    bool lightOn;

    char** resPaths;
    char** resTypes;
    uint16_t numRes;

    //Server only
    cometos_v6::CoAPResource* serverResources[2];
};

#endif /* COAPTEST_H_ */
