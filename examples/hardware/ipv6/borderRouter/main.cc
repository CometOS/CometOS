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

#include <sstream>
#include <iostream>
#include <iomanip>
#include "assert.h"
// cometos
#include "cometos.h"
#include "ICMPv6Message.h"
#include "Task.h"
#include "palId.h"
#include "IPv6Datagram.h"
#include "IPHCCompressor.h"
#include "IPHCDecompressor.h"
#include "LowpanBuffer.h"
#include "Ieee802154MacAddress.h"
#include "UDPLayer.h"
#include "UDPPacket.h"
#include "ContentRequest.h"
#include "ContentResponse.h"
#include "SerialComm.h"
#include "AsyncTcpComm.h"
#include "LowpanDispatcher.h"
#include "Module.h"
#include "IpForward.h"
#include "LowpanAdaptionLayer.h"

#include "StaticRouting.h"
#include "NeighborDiscovery.h"
#include "IPv6InterfaceTable.h"
#include "SimpleRouting.h"

#include "FwdBounce.h"
#include "AsyncTcpAgent.h"


//tuntap
#include "TunTap.h"

// BR
#include "BorderRouter.h"


// old
static cometos_v6::LowpanAdaptionLayer lowpan(LOWPAN_MODULE_NAME);
static cometos_v6::LowpanDispatcher<1> lowDisp;
static cometos::SimpleRouting routing;

static cometos_v6::RoutingTable rt(ROUTING_TABLE_MODULE_NAME);
static cometos_v6::NeighborDiscovery nd(NEIGHBOR_DISCOVERY_MODULE_NAME);
static cometos_v6::StaticRouting sr(ROUTING_MODULE_NAME);
static cometos_v6::IPv6InterfaceTable it(INTERFACE_TABLE_MODULE_NAME);

static FwdBounce bounce("sb", 0, true);
static AsyncTcpComm async;

// ############## new



//TcpComm comm;

int main(int argc, char* argv[]) {

    const char bs_comport[15] = "/dev/ttyUSB0"; // standard comport
    uint16_t portNo = 20000;

    SerialComm* comm = NULL;
    AsyncTcpComm* tcpc = NULL;

    if (argc > 2 && argc < 5 && strcmp(argv[2], "s") == 0) {
            if(argc==4) {
                const char* port = argv[3];
                comm = new SerialComm(port);
            } else {
                comm = new SerialComm(bs_comport);
            }
            lowDisp.gateReqOut.connectTo(comm->gateReqIn);
            comm->gateIndOut.connectTo(lowDisp.gateIndIn);
    } else if (argc > 2 && argc < 5 && strcmp(argv[2], "t") == 0) {
            if (argc == 4) {
                portNo = atoi(argv[3]);
            }
            tcpc = new AsyncTcpComm();
            tcpc->listen(portNo);
            lowDisp.gateReqOut.connectTo(tcpc->gateReqIn);
            tcpc->gateIndOut.connectTo(lowDisp.gateIndIn);
    } else {
        printf("Usage: %s IP-Prefix [s [serialPort]|t [TCPPort]]\n\ts\tuse serial port (standard %s)\n\tt\tuse tcp port (standard %d)\n",
                argv[0], bs_comport, portNo);
        return 0;
    }

    BorderRouter *br = new BorderRouter(argv[1]);
    br->toIPv4 = false;

    async.listen(20000);

    printf("Inner Address Prefix: %s\n", argv[1]);

//  ########### OLD

    br->toLowpan.connectTo(lowpan.fromIP);
    lowpan.toIP.connectTo(br->fromLowpan);

    lowpan.toMAC.connectTo(lowDisp.LowpanIn);
    lowDisp.LowpanOut.connectTo(lowpan.fromMAC);

//    routing.gateReqOut.connectTo(lowDisp.gateReqIn.get(0));
//    lowDisp.gateIndOut.get(0).connectTo(routing.gateIndIn);

    bounce.gateReqOut.connectTo(lowDisp.gateReqIn.get(0));
    lowDisp.gateIndOut.get(0).connectTo(bounce.gateIndIn);

    bounce.toSecondary.connectTo(async.gateReqIn);
    async.gateIndOut.connectTo(bounce.fromSecondary);

// #############



// ############

    /** cometOS **/
    cometos::initialize();

    // only for TCPComm
//    comm.connect("localhost", 20001);

    cometos::run();

    if (tcpc != NULL) {
        delete tcpc;
    }
    if (comm != NULL) {
        delete comm;
    }
    delete br;

    return 0;
}
