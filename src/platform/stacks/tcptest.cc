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
 *
 * Stack:
 *
 *                 RemoteAccess
 *                      |
 *    open   OTAP  RealibilityLayer  	PrintApp	 AssociationsService
 *     |0_____|1________|2________________|3_______________|4
 *                       |
 *                 Dispatcher<5>
 *                       |
 *  	open       SimpleRouting
 *        |0_____________|1
 *                |
 *           Dispatcher<2>
 *                |
 *             TcpComm
 *
 * @author Stefan Unterschuetz
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "protocolstack.h"
#include "cometos.h"
#include "Endpoint.h"
#include "Dispatcher.h"
#include "TcpComm.h"
#include "RemoteAccess.h"
#include "SimpleRouting.h"
#include "SystemMonitor.h"
#include "SimpleReliabilityLayer.h"
#include "PrintfApp.h"
#include "AssociationService.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

// Protocols
TcpComm comm;
Dispatcher<2> macDisp;
Dispatcher<5> nwkDisp;
SimpleRouting routing;
SystemMonitor sys;
//SimpleReliabilityLayer reliability;
RemoteAccess remote;
PrintfApp printfApp;
AssociationService as;

void protocolstack_init() {
	comm.gateIndOut.connectTo(macDisp.gateIndIn);
	macDisp.gateReqOut.connectTo(comm.gateReqIn);

	macDisp.gateIndOut.get(1).connectTo(routing.gateIndIn);
	routing.gateReqOut.connectTo(macDisp.gateReqIn.get(1));

	// Layer 3
	routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
	nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

	// Layer 4

	//nwkDisp.gateIndOut.get(2).connectTo(reliability.gateIndIn);
	//reliability.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));

    nwkDisp.gateIndOut.get(2).connectTo(remote.gateIndIn);
    remote.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));


	nwkDisp.gateIndOut.get(3).connectTo(printfApp.gateIndIn);
	printfApp.gateReqOut.connectTo(nwkDisp.gateReqIn.get(3));

	nwkDisp.gateIndOut.get(4).connectTo(as.gateIndIn);
	as.gateReqOut.connectTo(nwkDisp.gateReqIn.get(4));

	// Layer 5
//	reliability.gateIndOut.connectTo(remote.gateIndIn);
//	remote.gateReqOut.connectTo(reliability.gateReqIn);
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

