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
#include "TrafficExample.h"
#include "palLed.h"
#include "palId.h"
#include "OutputStream.h"
#include "logging.h"
#include "SList.h"
#include "DSMEPlatform.h"

#include "palPin.h"

#ifndef PAN_COORDINATOR
#define PAN_COORDINATOR false
#endif

/*PROTOTYPES-----------------------------------------------------------------*/

using namespace cometos;

class TestGateHandler : public Module{
public:
    void handle(DataIndication* msg) {
        cometos::getCout() << "MSG!" << cometos::endl;
        return;
    }
};

void setBlockingCout(bool value);

int main() {
    setBlockingCout(true);

    TrafficExample* traffic;

    getCout() << "Booting" << endl;

    if (PAN_COORDINATOR) {
        /* set up destinations */
        StaticSList<node_t, 10> dests;
        //dests.push_front(0x101A);
        dests.push_front(0x2856);

        /* instantiate traffic modules */
        traffic = new TrafficExample(dests, 24, 5000, 500, false);
    }

    dsme::DSMEPlatform dsme("mac");
    dsme.setLogLevel(LOG_LEVEL_DEBUG);

    if (PAN_COORDINATOR) {
        /* connect gates */
        traffic->gateReqOut.connectTo(dsme.gateReqIn);
        dsme.gateIndOut.connectTo(traffic->gateIndIn);

        traffic->setLogLevel(LOG_LEVEL_DEBUG);
    } else {
        Endpoint testEndpoint;
        TestGateHandler *testHandler = new TestGateHandler();
        InputGate<DataIndication> gate(testHandler, &TestGateHandler::handle, "Test");
        dsme.gateIndOut.connectTo(gate);
    }

    /* customizing CometOS's logging facility */
    cometos::setRootLogLevel(LOG_LEVEL_DEBUG);

    //getCout() << "Booted" << endl;

    /* start system */
    cometos::initialize();

    getCout() << "Booted " << hex << palId_id() << endl;

    cometos::run();
    return 0;
}
