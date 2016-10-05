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
 * Derived from the node example, but adds the time synchronization module
 *
 * @author Andreas Weigel
 */
/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "protocolstack.h"
#include "RemoteModule.h"
#include "palLed.h"
#include "Task.h"
#include "OutputStream.h"
#include "TimeSyncService.h"
#include "TxPowerLayer.h"
#include "palLocalTime.h"
#include "NetworkTime.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

class MyModule: public Module {
public:

    static const timeOffset_t INTERVAL = 1000;

    MyModule(const char* name) :
            Module(name) {
    }

    void initialize() {
        Module::initialize();
        tss = (TimeSyncService *) getModule(TimeSyncService::MODULE_NAME);
        ASSERT(tss != NULL);

        scheduleLed();
    }

    void toggleLed(Message * msg) {
//        getCout() << "t=" << tss->getMasterTime(palLocalTime_get()) << cometos::endl;
        getCout() << "t=" << NetworkTime::get() << cometos::endl;
//        time_ms_t mt = tss->getMasterTime(palLocalTime_get());
        time_ms_t mt = NetworkTime::get();
        if ((mt / INTERVAL) % 2 == 1) {
            palLed_on(1);
        } else {
            palLed_off(1);
        }
        scheduleLed();
    }

    void scheduleLed() {
//        time_ms_t mt = tss->getMasterTime(palLocalTime_get());
        time_ms_t mt = NetworkTime::get();
        timeOffset_t delay = INTERVAL - (mt % INTERVAL);
        if (delay == 0) {
            delay = INTERVAL;
        }
        schedule(&ledMsg, &MyModule::toggleLed, delay);
    }



private:
    TimeSyncService * tss;
    Message ledMsg;
};

static MyModule module("mod");


static TimeSyncService tss(TimeSyncService::MODULE_NAME, false, 1);
static Dispatcher<2> tsDisp;


int main() {
	protocolstack_init();

	tss.gateInitialOut.connectTo(tsDisp.gateReqIn.get(0));
	tsDisp.gateIndOut.get(0).connectTo(tss.gateInitialIn);

	tss.gateTimestampOut.connectTo(tsDisp.gateReqIn.get(1));
    tsDisp.gateIndOut.get(1).connectTo(tss.gateTimestampIn);

    protocolstack_macConnect(tsDisp.gateIndIn, tsDisp.gateReqOut);

	tss.setLogLevel(LOG_LEVEL_INFO);
	setRootLogLevel(LOG_LEVEL_INFO);

	cometos::initialize();
	protocolstack_sysMonInit();

	getCout() << "Starting...";

	cometos::run();
	return 0;
}

