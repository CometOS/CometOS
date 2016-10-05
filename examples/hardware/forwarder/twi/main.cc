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


/**I2C forwarder for connecting node with base station/OMNeT++.
 *
 * Can currently only build on board=local
 *
 * */

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "palLed.h"
#include "palId.h"
#include "TwiComm.h"
#include "Endpoint.h"
#include "AsyncTcpComm.h"
#include "FwdBounce.h"


#include <iostream>

using namespace cometos;
using namespace std;

/*PROTOTYPES-----------------------------------------------------------------*/

#define TWI_FWD_ALIVE_INTERVAL 10000

// sign of life output
TaskScheduler scheduler;
void toggleLed();
SimpleTask task(toggleLed);
void toggleLed() {
	static int val=0;
	std::cout<<"Still alive ("<<(++val)*(TWI_FWD_ALIVE_INTERVAL/1000)<<" seconds)"<<std::endl;
	cometos::getScheduler().add(task, TWI_FWD_ALIVE_INTERVAL);
}


// module instantiation


int main(int argc, const char *argv[]) {
	if (argc != 4 && argc != 5) {
	    std::cout<<"Client-Mode: executable <I2C_PORT> <I2C_SLAVE_ADDRESS> <IP_ADDRESS> <PORT>"<<std::endl;
	    std::cout<<"Server-Mode: executable <I2C_PORT> <I2C_SLAVE_ADDRESS> <PORT>"<<std::endl;
		return EXIT_FAILURE;
	}


	TwiComm comm(argv[1], true, atoi(argv[2]));
	AsyncTcpComm tcp;
	FwdBounce bounce("sb", 0, true);


	bounce.gateReqOut.connectTo(comm.gateReqIn);
	comm.gateIndOut.connectTo(bounce.gateIndIn);

	bounce.toSecondary.connectTo(tcp.gateReqIn);
	tcp.gateIndOut.connectTo(bounce.fromSecondary);


	cometos::initialize();

	if (argc == 3) {
		std::cout<<"Server-Mode"<<std::endl;
		tcp.listen(atoi(argv[3]));
	} else if (argc == 4) {
	    std::cout<<"Client-Mode"<<std::endl;
		tcp.connect(argv[3],atoi(argv[4]));
	}
	// if needed we can easily implement a reconnect


	cometos::getScheduler().add(task, TWI_FWD_ALIVE_INTERVAL);

	cometos::run();
	return 0;
}

