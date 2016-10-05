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
#include "Otap.h"
#include "avr/wdt.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

SerialComm comm;
SimpleCsmaMac mac;
NetworkInterfaceSwitch proxy(NULL, 0x0100, 0xfffe);

Dispatcher<3> macDisp;
Dispatcher<3> nwkDisp;
PeriodicTraffic traffic1;
PeriodicTraffic traffic2;
RemoteAccess remote;
Routing routing;
SystemMonitor sys;
CommAssessment ca("ca");
Otap otap;

/* Using this as status LED*/
void toggleLed();

SimpleTask task(toggleLed);
TaskScheduler scheduler;

void toggleLed() {
	palLed_toggle(1);
	cometos::getScheduler().add(task, 500);
	wdt_reset();
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

	macDisp.gateIndOut.get(1).connectTo(traffic1.gateIndIn);
	traffic1.gateReqOut.connectTo(macDisp.gateReqIn.get(1));

	macDisp.gateIndOut.get(2).connectTo(ca.gateIndIn);
	ca.gateReqOut.connectTo(macDisp.gateReqIn.get(2));

	routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
	nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

	nwkDisp.gateIndOut.get(0).connectTo(remote.gateIndIn);
	remote.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));

	nwkDisp.gateIndOut.get(1).connectTo(traffic2.gateIndIn);
	traffic2.gateReqOut.connectTo(nwkDisp.gateReqIn.get(1));

	nwkDisp.gateIndOut.get(2).connectTo(otap.gateIndIn);
	otap.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));


	cometos::initialize();
	//traffic1.start(2000);
	//traffic2.start(2000);

	cometos::getScheduler().add(task, 500);
	cometos::run();
	return 0;
}

