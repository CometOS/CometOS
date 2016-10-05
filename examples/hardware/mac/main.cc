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

/**@file This basic example shows the MAC usage.
 *
 * @author Stefan Unterschuetz
 */

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "CsmaMac.h"
#include "TrafficExample.h"
#include "palLed.h"
#include "palId.h"
#include "OutputStream.h"
#include "cometosAssert.h"
#include "logging.h"
#include "pal.h"

/*PROTOTYPES-----------------------------------------------------------------*/

using namespace cometos;

int main() {
    StaticSList<node_t, 10> dests;
    dests.push_front(MAC_BROADCAST);
    dests.push_front(0xb680);
    dests.push_front(0xc268);

    // instantiating modules
	TrafficExample traffic(dests, 24, 5000, 500, true);
	CsmaMac csma("mac");

	// connecting gates
	traffic.gateReqOut.connectTo(csma.gateReqIn);
	csma.gateSnoopIndOut.connectTo(traffic.gateSnoopIn);
	csma.gateIndOut.connectTo(traffic.gateIndIn);

	// customizing CometOS's logging facility
	traffic.setLogLevel(LOG_LEVEL_DEBUG);
	cometos::setRootLogLevel(LOG_LEVEL_DEBUG);

	cometos::initialize();
	LOG_INFO("Start node " << hex << palId_id());

	cometos::run();

	cometos::getCout() << "Reached End of program --> shutting down \n";
	return 0;
}
