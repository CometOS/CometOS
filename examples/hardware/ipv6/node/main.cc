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

#include "cometos.h"
#include "Platform.h"
#include "palId.h"
#include "pal.h"
#include "palLed.h"
#include "palFirmware.h"
#include "Task.h"

#include "Otap.h"
#include "SerialComm.h"
#include "NetworkInterfaceSwitch.h"

#include "CsmaMac.h"
#include "Dispatcher.h"
#include "SimpleRouting.h"

#include "RemoteAccess.h"
#include "SystemMonitor.h"
#include "PrintfApp.h"

#include "TrafficGen.h"

#include "RoutingTable.h"
#include "StaticRouting.h"
#include "NeighborDiscovery.h"
#include "IPv6InterfaceTable.h"
#include "TimeSyncService.h"


#include "ICMPv6.h"
#include "UDPLayer.h"
#include "UnknownProtocol.h"

#include "IpForward.h"
#include "LowpanAdaptionLayer.h"
#include "LowpanDispatcher.h"
#include "TxPowerLayer.h"
#include "TopologyMonitor.h"
#include "SimpleReliabilityLayer.h"

#include "CFSParameterStore.h"
#include "CFSFormat.h"


using namespace cometos;

#ifndef LOWPAN_DEFAULT_POWER
#define LOWPAN_DEFAULT_POWER 0xFF
#endif

#ifdef PAL_WDT
#include "Heartbeat.h"
static Heartbeat wd("wd");
#endif

static cometos_v6::IPv6InterfaceTable it(INTERFACE_TABLE_MODULE_NAME);
static cometos_v6::RoutingTable rt(ROUTING_TABLE_MODULE_NAME);

static cometos_v6::StaticRouting sr(ROUTING_MODULE_NAME);

static cometos_v6::NeighborDiscovery nd(NEIGHBOR_DISCOVERY_MODULE_NAME);

static TrafficGen mTest(TrafficGen::MODULE_NAME);

static cometos_v6::ICMPv6 icmp(ICMP_MODULE_NAME);

static cometos_v6::UDPLayer udp(UDP_MODULE_NAME);
static cometos_v6::UnknownProtocol ukp("ukp");

static cometos_v6::IpForward ipfwd(IPFWD_MODULE_NAME);

static cometos_v6::LowpanAdaptionLayer lowpan(LOWPAN_MODULE_NAME);

#ifdef PAL_FIRMWARE_ASYNC
#include "OtapAsync.h"
static OtapAsync otap;
#elif defined PAL_FIRMWARE
#include "Otap.h"
static Otap otap;
#endif

static CsmaMac mac(MAC_MODULE_NAME);
static Dispatcher<4> macDisp;
static Dispatcher<4> macDisp2;
static Dispatcher<4> nwkDisp;
static TimeSyncService tss(TimeSyncService::MODULE_NAME, 5, 50, 11, 5, false);
static Dispatcher<2> tsDisp;
static cometos_v6::LowpanDispatcher<1> lowDisp;
static cometos_v6::LowpanDispatcher<1> lowDisp2("lodi");
static TopologyMonitor<TM_LIST_SIZE> tm("tm");

static SimpleRouting routing;
static RemoteAccess remote;
static SystemMonitor sysMon(SystemMonitor::DEFAULT_MODULE_NAME, &remote);
#ifndef SERIAL_PRINTF
static PrintfApp printi("pa", BASESTATION_ADDR);
#endif
static TxPowerLayer txp(TX_POWER_LAYER_MODULE_NAME, LOWPAN_DEFAULT_POWER);
static SimpleReliabilityLayer srl(RELIABILITY_MODULE_NAME);

static CFSFormat cfsf;
static CFSParameterStore configStorage;
//static FileManager fm(FILE_MANAGER_MODULE_NAME);

// TODO just to initially write the correct address
//#include <avr/eeprom.h>
//#include <avr/wdt.h>
int main() {
    palLed_init();
    mac_disableAutomaticDiversity(1);

// in case board ID needs to be set
//    uint16_t addr = 0x0102;     // last four digits of MAC printed on board
//    eeprom_write_word((uint16_t*) 0, addr);

//---------- ip stack ---------------------------------------------------------
    // between icmp and ipfwd
    icmp.toIP.connectTo(ipfwd.fromICMP);
    ipfwd.toICMP.connectTo(icmp.fromIP);

    // between udp and ipfwd
    udp.toIP.connectTo(ipfwd.fromUDP);
    ipfwd.toUDP.connectTo(udp.fromIP);

    // between ukp and ipfwd
//    ukp.toIP.connectTo(ipfwd.fromUnknown);
    ipfwd.toUnknown.connectTo(ukp.fromIP);

    // between ipfwd and lowpan
    ipfwd.toLowpan.connectTo(lowpan.fromIP);
    lowpan.toIP.connectTo(ipfwd.fromLowpan);

    // between lowpan and mac
    lowpan.toMAC.connectTo(lowDisp.LowpanIn);
    lowDisp.LowpanOut.connectTo(lowpan.fromMAC);

    tm.gateReqOut.connectTo(lowDisp.gateReqIn.get(0));
    lowDisp.gateIndOut.get(0).connectTo(tm.gateIndIn);

    lowDisp.gateReqOut.connectTo(txp.gateReqIn);
    txp.gateIndOut.connectTo(lowDisp.gateIndIn);

    txp.gateReqOut.connectTo(macDisp.gateReqIn.get(1));
    macDisp.gateIndOut.get(1).connectTo(txp.gateIndIn);

//---------- flooding routing stack for remote access -------------------------
#ifndef SERIAL_PRINTF
// Printf
    printi.gateReqOut.connectTo(nwkDisp.gateReqIn.get(1));
    nwkDisp.gateIndOut.get(1).connectTo(printi.gateIndIn);
#endif

// Remote Access (w/ dispatcher)
    remote.gateReqOut.connectTo(srl.gateReqIn);
    srl.gateIndOut.connectTo(remote.gateIndIn);

    srl.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));
    nwkDisp.gateIndOut.get(0).connectTo(srl.gateIndIn);
#if defined PAL_FIRMWARE or defined PAL_FIRMWARE_ASYNC
// Otap
	otap.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));
	nwkDisp.gateIndOut.get(2).connectTo(otap.gateIndIn);
#endif
	
// Network Dispatcher
    nwkDisp.gateReqOut.connectTo(routing.gateReqIn);
    routing.gateIndOut.connectTo(nwkDisp.gateIndIn);

// Routing (w/ dispatcher)
    routing.gateReqOut.connectTo(macDisp.gateReqIn.get(0));
    macDisp.gateIndOut.get(0).connectTo(routing.gateIndIn);

	// TimeSyncService above mac
    tss.gateInitialOut.connectTo(tsDisp.gateReqIn.get(0));
    tsDisp.gateIndOut.get(0).connectTo(tss.gateInitialIn);

    tss.gateTimestampOut.connectTo(tsDisp.gateReqIn.get(1));
    tsDisp.gateIndOut.get(1).connectTo(tss.gateTimestampIn);

    tsDisp.gateReqOut.connectTo(macDisp.gateReqIn.get(2));
    macDisp.gateIndOut.get(2).connectTo(tsDisp.gateIndIn);

//---------- mac and proxy ----------------------------------------------------
    macDisp.gateReqOut.connectTo(mac.gateReqIn);
    mac.gateIndOut.connectTo(macDisp.gateIndIn);

    mac.gateSnoopIndOut.connectTo(macDisp2.gateIndIn);
    macDisp2.gateIndOut.get(1).connectTo(lowDisp2.gateIndIn);
    lowDisp2.LowpanOut.connectTo(lowpan.macSnoop);

//---------------

    cometos::initialize();
    tss.setLogLevel(LOG_LEVEL_ERROR);
    mac.setLogLevel(LOG_LEVEL_ERROR);
    setRootLogLevel(LOG_LEVEL_ERROR);
    LOG_ERROR("Starting");
    cometos::run();

    return 0;
}
