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
 * @author Gerry Siegemund
 */

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "CsmaMac.h"
#include "TCPWY.h"
#include "LWT.h"
#include "palLed.h" //What's that good for?
#include "palId.h"
#include "OutputStream.h"
#include "cometosAssert.h"
#include "logging.h"
#include "pal.h"

//#define ALGORITHM_MIS
#ifdef ALGORITHM_MIS
#include "MISAlgo.h"
#endif

//#define ALGORITHM_SPT
#ifdef ALGORITHM_SPT
#include "SPTAlgo.h"
#endif

#define ALGORITHM_COLOR
#ifdef ALGORITHM_COLOR
#include "ColorAlgo.h"
#endif

//#define OUTPUT_KOTTBUS
#define OUTPUT_WEIGELT
/*PROTOTYPES-----------------------------------------------------------------*/

using namespace cometos;

int main() {
    // instantiating modules
	TCPWY tcpwy;
	LWT lwt;
#ifdef ALGORITHM_MIS
	MISAlgo* algorithm = new MISAlgo();
#endif
#ifdef ALGORITHM_SPT
    SPTAlgo* algorithm = new SPTAlgo();
#endif
#ifdef ALGORITHM_COLOR
    ColorAlgo* algorithm = new ColorAlgo();
#endif
	CsmaMac csma("mac");

//    cometos::getCout() << "Start a new program! \n";
	// connecting gates
//        mac.gateIndOut --> tcpwy.gateIndIn;
//        tcpwy.gateReqOut --> mac.gateReqIn;

	csma.gateIndOut.connectTo(tcpwy.gateIndIn);
	tcpwy.gateReqOut.connectTo(csma.gateReqIn);
	tcpwy.gateIndOut.connectTo(lwt.gateIndIn);
	lwt.gateReqOut.connectTo(tcpwy.gateReqIn);
	tcpwy.pGateControlOut.connectTo(lwt.pGateControlIn);
	lwt.pGateControlOut.connectTo(tcpwy.pGateControlIn);

	lwt.setNetworkModul(&tcpwy);
	lwt.setAlgorithm(algorithm);

	// customizing CometOS's logging facility
//	tcpwy.setLogLevel(LOG_LEVEL_DEBUG);
	cometos::setRootLogLevel(LOG_LEVEL_DEBUG);

	cometos::initialize();
//	LOG_INFO("Start node " << hex << palId_id());

	cometos::getCout() << "TEST! \n";

	cometos::run();

	cometos::getCout() << "Reached End of program --> shutting down \n";
	return 0;
}
