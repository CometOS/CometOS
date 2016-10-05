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


/**Serial forwarder for connecting node with base station/OMNeT++.
 * Note that a more sophisticated forwarder can be found for
 * python, which includes a packet analyzer.
 *
 * Can currently only build on board=local
 *
 * */

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "palLed.h"
#include "palId.h"
#include "Endpoint.h"
#include "AsyncTcpComm.h"
#include "FwdBounce.h"
#include "SerialDispatchCpp.cc"

#include <iostream>
#include <cstring>
#include <map>

using namespace cometos;
using namespace std;

/*PROTOTYPES-----------------------------------------------------------------*/

#define SERIAL_FWD_ALIVE_INTERVAL 10000

// sign of life output
TaskScheduler scheduler;
void toggleLed();
SimpleTask task(toggleLed);
void toggleLed() {
	static int val=0;
	//std::cout<<"Still alive ("<<(++val)*(SERIAL_FWD_ALIVE_INTERVAL/1000)<<" seconds)"<<std::endl;
	cometos::getScheduler().add(task, SERIAL_FWD_ALIVE_INTERVAL);
}

void usage() {
    std::cout << "tcp_to_multi_serial <tcp-listen-port> <serial baudrate> <serial frame timeout> <commport,node-uid+commport,node-uid+...>" << std::endl;;
}

bool getNodePortMappingFromArgv(char * firstArg, std::map<node_t, const char* >& nodemap) {
    node_t n;
    char * portstr = strtok(firstArg, ",+");
    if (portstr != NULL) {
        char * nstr = strtok(NULL, ",+");
        if (nstr != NULL) {
            n = (node_t) atoi(nstr);
        } else {
            return false;
        } 
    } else {
        return false;
    }
    std::cout << "add node/port mapping; node=" << n << " port=" << portstr << std::endl;
    nodemap[n] = portstr;
    return true;
}

// module instantiation

int main(int argc, const char *argv[]) {
    static std::map<node_t, const char*> nodemap; 
    static char * mapstr = new char[strlen(argv[4])];
    strncpy(mapstr, argv[4], strlen(argv[4]));
	if (argc != 5) {
        usage();
        return EXIT_FAILURE;
	}
    uint32_t baudrate = atoi(argv[2]); 
    if (baudrate == 0) {
        usage();
        return EXIT_FAILURE;
    }
    uint16_t frameTo = atoi(argv[3]);
    if (frameTo == 0) {
        usage();
        return EXIT_FAILURE;
    }
    if (!getNodePortMappingFromArgv(mapstr, nodemap)) {
        usage();
        return EXIT_FAILURE;
    }
    bool cont = true;
    while(cont) {
        cont = getNodePortMappingFromArgv(NULL, nodemap);
    }
    
    std::cout << "creating SerialDispatch with baudrate=" << baudrate << " frameTimeout=" << frameTo << std::endl; 
	SerialDispatchCpp comm(nodemap, baudrate, frameTo);
	AsyncTcpComm tcp;
	FwdBounce bounce("sb", 0, false);


	bounce.gateReqOut.connectTo(comm.gateReqIn);
	comm.gateIndOut.connectTo(bounce.gateIndIn);

	bounce.toSecondary.connectTo(tcp.gateReqIn);
	tcp.gateIndOut.connectTo(bounce.fromSecondary);


	cometos::initialize();

    std::cout<<"Server-Mode"<<std::endl;
    tcp.listen(atoi(argv[1]));


	cometos::getScheduler().add(task, SERIAL_FWD_ALIVE_INTERVAL);

	cometos::run();
	return 0;
}

