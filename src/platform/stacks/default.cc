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

/**This file contains the instantiation and configuration of the
 * CometOS Default-Stack
 *
 * Features:
 * <li> simple routing functionality
 * <li> remote method invocation support (with end-to-end reliability)
 * <li> multi-hop printf support
 * <li> multi-hop over-the-air-programming
 * <li> CsmaMac and SerialComm as network interfaces
 * <li> supports one user layer3 and layer4 protocol
 *
 * Stack:
 *
 *                 RemoteAccess
 *                      |
 *    open   OTAP  RealibilityLayer  	PrintApp	 AssociationsService
 *     |0_____|1________|2________________|3_______________|4
 *                       |
 *                 Dispatcher<5>	TopologyMonitor
 *                       |				 |
 *  	open       SimpleRouting    txPowerLayer
 *        |0_____________|1______________|2
 *                |
 *           Dispatcher<4>
 *           	  |
 *      NetworkInterfaceSwitch
 *       _________|_______
 *       |cni            |nni
 *   SimpleCsmaMac    SerialComm
 *
 * @author Stefan Unterschuetz, Andreas Weigel
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "protocolstack.h"
#include "cometos.h"
#include "SerialComm.h"
#include "Endpoint.h"
#include "SerialComm.h"
#include "Dispatcher.h"
#include "RemoteAccess.h"
#include "SimpleRouting.h"
#include "SystemMonitor.h"
#include "TxPowerLayer.h"
#include "CsmaMac.h"
#include "NetworkInterfaceSwitch.h"
#include "SimpleReliabilityLayer.h"
#include "Heartbeat.h"
#include "TimeSyncService.h"

#ifndef SERIAL_PRINTF
#include "PrintfApp.h"
#endif
#include "AssociationService.h"
#include "TopologyMonitor.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/


// Protocols
#ifndef SERIAL_PRINTF
SerialComm comm(SERIAL_COMM_PORT);
NetworkInterfaceSwitch proxy("nis", 0x0100, 0xfffe);
#endif
CsmaMac mac("mac");
Dispatcher<4> macDisp;
Dispatcher<5> nwkDisp;
SimpleRouting routing("sro");
RemoteAccess remote;
SystemMonitor sys(SystemMonitor::DEFAULT_MODULE_NAME, &remote);

#ifdef PAL_FIRMWARE_ASYNC
    #include "OtapAsync.h"
    OtapAsync otap;
#else
    #ifdef PAL_FIRMWARE
        #include "Otap.h"
        Otap otap;
    #endif
#endif

SimpleReliabilityLayer reliability("sr");

#ifndef SERIAL_PRINTF
PrintfApp printfApp("pa");
#else
Endpoint printfApp("pa");
#endif
AssociationService as;
static TopologyMonitor<TM_LIST_SIZE> topo("tm");
static TxPowerLayer txp(TX_POWER_LAYER_MODULE_NAME, 0xFF);

static TimeSyncService t;
static Heartbeat w("wdt");

void protocolstack_init() {
#ifndef SERIAL_PRINTF
	// Layer 2
	mac.gateIndOut.connectTo(proxy.cniIndIn);
	proxy.cniReqOut.connectTo(mac.gateReqIn);

	comm.gateIndOut.connectTo(proxy.nniIndIn);
	proxy.nniReqOut.connectTo(comm.gateReqIn);

	proxy.gateIndOut.connectTo(macDisp.gateIndIn);
	macDisp.gateReqOut.connectTo(proxy.gateReqIn);

#else
	mac.gateIndOut.connectTo(macDisp.gateIndIn);
	macDisp.gateReqOut.connectTo(mac.gateReqIn);
#endif
	macDisp.gateIndOut.get(1).connectTo(routing.gateIndIn);
	routing.gateReqOut.connectTo(macDisp.gateReqIn.get(1));

//	macDisp.gateIndOut.get(2).connectTo(topo.gateIndIn);
//	topo.gateReqOut.connectTo(macDisp.gateReqIn.get(2));

	topo.gateReqOut.connectTo(txp.gateReqIn);
	txp.gateIndOut.connectTo(topo.gateIndIn);

	txp.gateReqOut.connectTo(macDisp.gateReqIn.get(2));
	macDisp.gateIndOut.get(2).connectTo(txp.gateIndIn);

	// Layer 3
	routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
	nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

	// Layer 4
#if defined(PAL_FIRMWARE) || defined(PAL_FIRMWARE_ASYNC)
	nwkDisp.gateIndOut.get(1).connectTo(otap.gateIndIn);
	otap.gateReqOut.connectTo(nwkDisp.gateReqIn.get(1));
#endif

	nwkDisp.gateIndOut.get(2).connectTo(reliability.gateIndIn);
	reliability.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));

//#ifndef SERIAL_PRINTF
	//macDisp.gateIndOut.get(0).connectTo(printfApp.gateIndIn);
	//printfApp.gateReqOut.connectTo(macDisp.gateReqIn.get(0));

	nwkDisp.gateIndOut.get(3).connectTo(printfApp.gateIndIn);
	printfApp.gateReqOut.connectTo(nwkDisp.gateReqIn.get(3));
//#endif
	nwkDisp.gateIndOut.get(4).connectTo(as.gateIndIn);
	as.gateReqOut.connectTo(nwkDisp.gateReqIn.get(4));

	// Layer 5
	reliability.gateIndOut.connectTo(remote.gateIndIn);
	remote.gateReqOut.connectTo(reliability.gateReqIn);
}

void protocolstack_nwkConnect(InputGate<DataIndication>& indIn,
		OutputGate<DataRequest>& reqOut) {
	nwkDisp.gateIndOut.get(0).connectTo(indIn);
	reqOut.connectTo(nwkDisp.gateReqIn.get(0));
}

void protocolstack_macConnect(InputGate<DataIndication>& indIn,
		OutputGate<DataRequest>& reqOut) {
	macDisp.gateIndOut.get(0).connectTo(indIn);
	reqOut.connectTo(macDisp.gateReqIn.get(0));
}

void protocolstack_sysMonInit() {
    LOG_INFO(" starting node ...");
}
