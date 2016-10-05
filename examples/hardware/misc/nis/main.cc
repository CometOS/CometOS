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
#include "palId.h"
#include "SerialComm.h"
#include "Endpoint.h"
#include "pal.h"

#include "SerialComm.h"
#include "Dispatcher.h"
#include "PeriodicTraffic.h"
#include "RemoteAccess.h"
#include "Routing.h"
#include "SystemMonitor.h"
#include "SimpleCsmaMac.h"
#include "NetworkInterfaceSwitch.h"
#include "CommAssessment.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

SerialComm comm;
SimpleCsmaMac mac;
NetworkInterfaceSwitch proxy(NULL, 0x0100, 0xfffe);

Dispatcher<3> macDisp;
Dispatcher<2> nwkDisp;
RemoteAccess remote;
Routing routing;
SystemMonitor sys;

/* Using this as status LED*/
void toggleLed();

SimpleTask task(toggleLed);
TaskScheduler scheduler;

class LedControl: public RemoteModule {
public:
	LedControl(const char* chr) :
			RemoteModule(chr) {
	}
	void initialize() {
		Module::initialize();
		remoteDeclare(&LedControl::green, "g");
		remoteDeclare(&LedControl::green, "y");
		remoteDeclare(&LedControl::green, "r");
	}

	void green() {
		palLed_toggle(1);
	}

	void yellow() {
		palLed_toggle(2);
	}

	void red() {
		palLed_toggle(4);
	}

} ledControl("led");

void toggleLed() {
	palLed_toggle(1);
}

int main() {
	palLed_init();

	mac.gateIndOut.connectTo(proxy.cniIndIn);
	proxy.cniReqOut.connectTo(mac.gateReqIn);

	comm.gateIndOut.connectTo(proxy.nniIndIn);
	proxy.nniReqOut.connectTo(comm.gateReqIn);

	proxy.gateIndOut.connectTo(macDisp.gateIndIn);
	macDisp.gateReqOut.connectTo(proxy.gateReqIn);

	macDisp.gateIndOut.get(0).connectTo(routing.gateIndIn);
	routing.gateReqOut.connectTo(macDisp.gateReqIn.get(0));

	routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
	nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

	nwkDisp.gateIndOut.get(0).connectTo(remote.gateIndIn);
	remote.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));

	cometos::initialize();

	palLed_on(1);
	cometos::getScheduler().add(task, 1000);
	cometos::run();
	return 0;
}

