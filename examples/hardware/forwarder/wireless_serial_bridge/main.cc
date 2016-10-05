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
#include "SerialComm.h"
#include "CsmaMac.h"
#include "TimeSyncService.h"
#include "SerialComm.h"
#include "Dispatcher.h"
#include "RemoteAccess.h"
#include "SystemMonitor.h"
#include "CFSParameterStore.h"
#include "PrintfApp.h"
#include "Heartbeat.h"

static cometos::Vector<uint8_t, 2> ppini;
static cometos::Vector<uint8_t, 2> ppts;
static cometos::Vector<uint8_t, 1> ppra;

static node_t fakeAddress = 0x0;
static cometos::CsmaMac mac(MAC_MODULE_NAME, &fakeAddress);
static cometos::SerialComm comm(0);
static cometos::Dispatcher<5> macDisp;

static cometos::Heartbeat wd("wdt");

static cometos::RemoteAccess remote;
static cometos::SystemMonitor sysMon(cometos::SystemMonitor::DEFAULT_MODULE_NAME, &remote);
static cometos::TimeSyncWirelessBridge bridge(ppini, ppts, ppra, "brdg");
static PrintfApp printi;
static cometos::CFSParameterStore configStorage;
int main() {
    palLed_init();

    ppini.pushBack(2); ppini.pushBack(0);
    ppts.pushBack(2); ppts.pushBack(1);
    ppra.pushBack(3);

    // Remote Access (w/ dispatcher)
    remote.gateReqOut.connectTo(macDisp.gateReqIn.get(3));
    macDisp.gateIndOut.get(3).connectTo(remote.gateIndIn);

    printi.gateReqOut.connectTo(macDisp.gateReqIn.get(4));

//---------- mac and proxy ----------------------------------------------------
    macDisp.gateReqOut.connectTo(bridge.gateReqIn);
    bridge.gateIndOut.connectTo(macDisp.gateIndIn);

	bridge.gateReqOut.connectTo(mac.gateReqIn);
	mac.gateIndOut.connectTo(bridge.gateIndIn);

	mac.gateSnoopIndOut.connectTo(bridge.gateSnoopIndIn);

	bridge.toSerial.connectTo(comm.gateReqIn);
	comm.gateIndOut.connectTo(bridge.fromSerial);

	cometos::initialize();
	cometos::run();
	return 0;
}

