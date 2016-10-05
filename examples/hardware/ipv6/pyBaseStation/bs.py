#!/usr/bin/python
import sys
cometosDir = '../../../'
cometosPyPath = cometosDir + 'support/python/'

sys.path.append(cometosPyPath)
import time
import os
from modules.init_functions import setNode
from cometosMisc import *


def usage():
    print "usage: " + sys.argv[0] + " <serialport> <network> [hex(bsAttachedNodeId)]"
    
print len(sys.argv)
if len(sys.argv) != 3:
    usage()
    os._exit(1)

networkFile = sys.argv[2]


MAC_MODULE_NAME = "mac"
    
print "SETTING CONFIGS -- check correctness!"

cmdlog = redirect("./log/bs/" + time.strftime("%Y-%m-%d") + ".log")
bootLogName =  "./log/nodeEvents/" + time.strftime("%Y-%m-%d") + ".log"
sys.stdout = cmdlog
sys.stderr = cmdlog

import swig.cometos as cometos 
from cometosMisc import *

from stack.RemoteAccess import *
from stack.RemoteAccess import RemoteModule as pyRemoteModule
from stack.PyOtap import *
from stack.PySysMon import *
from stack.PyTopology import *

from modules.PyTrafficGen import PyTrafficGen
from modules.cfg import *

########### Constants #############################
IMG_DB="../dbg_node/devboard256Flash/Device.hex"
IMG_BS="../bs_node/devboard256Flash/Device.hex"
IMG_NODE="../../examples/node/devboard256Flash/Device.hex"
TEST="../../examples/blink/devboard256Flash/Device.hex"    

def stop():
    quit()
    
def ids():
    global nodes
    for i in range(0, len(nodes)):
        print str(i) + ": NodeId=" + hex(nodes[i].id) + "(" + str(nodes[i].id) + ")"

#########################################################################
# Instantiate modules of communication stack ############################
#########################################################################
# IPv6 stack
routingTable = cometos.RoutingTable(ROUTING_TABLE_MODULE_NAME)
staticRouting = cometos.StaticRouting(ROUTING_MODULE_NAME)
neighborDisc = cometos.NeighborDiscovery(NEIGHBOR_DISCOVERY_MODULE_NAME)
interfaceTable = cometos.IPv6InterfaceTable(INTERFACE_TABLE_MODULE_NAME)

icmp = cometos.ICMPv6(ICMP_MODULE_NAME)
udp = cometos.UDPLayer(UDP_MODULE_NAME)
ukp = cometos.UnknownProtocol("ukp")
ipfwd = cometos.IpForward(IPFWD_MODULE_NAME)
lowpan = cometos.LowpanAdaptionLayer(LOWPAN_MODULE_NAME)
loof = cometos.LowpanOrderedForwarding()
tm = cometos.TopologyMonitorX("tm")

# auxiliary stack
mac = cometos.SerialMac("mac")
txp = cometos.TxPowerLayer(TX_POWER_LAYER_MODULE_NAME)
comm = cometos.SerialComm(sys.argv[1])

#comm = cometos.AsyncTcpComm()
#comm.connect("localhost",20000)
macDisp = cometos.Dispatcher8()
lowpanDisp1 = cometos.LowpanDispatcher2("ldi1")
lowpanDisp2 = cometos.LowpanDispatcher2("ldi2")
tsDisp = cometos.Dispatcher2()
routing = cometos.SimpleRouting("srou")
printi = cometos.PrintfAppReceiver()
nwkDisp = cometos.Dispatcher4()
remote = RemoteAccess()
remote.setWaitingTime(3.5)
raGateway = RemoteAccess("ragw")
raGateway.setWaitingTime(0.5)
tss = cometos.TimeSyncService(cometos.TimeSyncService.MODULE_NAME, 5, 50, 12, 2, True)
srl = cometos.SimpleReliabilityLayer(RELIABILITY_MODULE_NAME)
otap = PyOtap()

tg = PyTrafficGen(cometos.TrafficGen.MODULE_NAME, tss)

sysMon = PySysMon(bootLogName)

bs=Node()
bs.id = BASESTATION_ADDR
bs.rm=staticRouting
bs.tg=tg
bs.udp=udp
bs.low=lowpan
bs.low.sc = bs.low.setConfig
bs.low.gc = bs.low.getConfig
bs.icmp=icmp
bs.tss = tss
bs.srl = srl
bs.tm = tm
bs.tm.gnl = bs.tm.getNodeList
bs.tm.gi = bs.tm.getInfo
bs.tm.gns = bs.tm.getNumSent
bs.txp = txp
bs.txp.spl = bs.txp.setPwrLvl
bs.txp.gpl = bs.txp.getPwrLvl
bs.txp.rpl = bs.txp.resetPwrLvl


################################################################################
# WIRING #######################################################################
################################################################################
icmp.toIP.connectTo(ipfwd.fromICMP);
udp.toIP.connectTo(ipfwd.fromUDP);

ipfwd.toUDP.connectTo(udp.fromIP);
ipfwd.toICMP.connectTo(icmp.fromIP);
ipfwd.toUnknown.connectTo(ukp.fromIP);
ipfwd.toLowpan.connectTo(lowpan.fromIP);

lowpan.toIP.connectTo(ipfwd.fromLowpan);

# connect time sync service
tss.gateInitialOut.connectTo(tsDisp.gateReqIn.get(0));
tsDisp.gateIndOut.get(0).connectTo(tss.gateInitialIn);

tss.gateTimestampOut.connectTo(tsDisp.gateReqIn.get(1));
tsDisp.gateIndOut.get(1).connectTo(tss.gateTimestampIn);

tsDisp.gateReqOut.connectTo(macDisp.gateReqIn.get(2));
macDisp.gateIndOut.get(2).connectTo(tsDisp.gateIndIn);

lowpan.toMAC.connectTo(lowpanDisp1.LowpanIn);
lowpanDisp1.LowpanOut.connectTo(lowpan.fromMAC);

tm.gateReqOut.connectTo(lowpanDisp1.gateReqIn.get(0));
lowpanDisp1.gateIndOut.get(0).connectTo(tm.gateIndIn);

lowpanDisp1.gateReqOut.connectTo(txp.gateReqIn);
txp.gateIndOut.connectTo(lowpanDisp1.gateIndIn);

txp.gateReqOut.connectTo(macDisp.gateReqIn.get(1));
macDisp.gateIndOut.get(1).connectTo(txp.gateIndIn);

remote.gateReqOut.connectTo(srl.gateReqIn);
srl.gateIndOut.connectTo(remote.gateIndIn);

srl.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));
nwkDisp.gateIndOut.get(0).connectTo(srl.gateIndIn);

otap.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));
nwkDisp.gateIndOut.get(2).connectTo(otap.gateIndIn);

nwkDisp.gateReqOut.connectTo(routing.gateReqIn);
routing.gateIndOut.connectTo(nwkDisp.gateIndIn);

routing.gateReqOut.connectTo(macDisp.gateReqIn.get(0));
macDisp.gateIndOut.get(0).connectTo(routing.gateIndIn);

raGateway.gateReqOut.connectTo(macDisp.gateReqIn.get(3));
macDisp.gateIndOut.get(3).connectTo(raGateway.gateIndIn);

macDisp.gateIndOut.get(4).connectTo(printi.gateIndIn);

macDisp.gateReqOut.connectTo(mac.gateReqIn);
mac.gateIndOut.connectTo(macDisp.gateIndIn);

mac.snoopIndOut.connectTo(lowpanDisp2.gateIndIn);
lowpanDisp2.LowpanOut.connectTo(lowpan.macSnoop);

mac.gateReqOut.connectTo(comm.gateReqIn);
comm.gateIndOut.connectTo(mac.gateIndIn);


# initialize CometOS
cometos.initialize()

# Load addtional runtime scripts
ex=Script([cometosPyPath + "/script", "scripts"],globals())


def asyncT(node, pingValue):
    print "PingEvent: " + str(pingValue) + " from " + str(node)

def initNode(id, remoteAccessModule):
    node = Node()
    node.id = id
    node.addModule(remoteAccessModule, MAC_MODULE_NAME, node.id)
    node.mac.declareMethod("rsc", uint8_t)
    node.mac.declareMethod("gsc", cometos.MacConfig)
    node.mac.declareMethod("gac", cometos.MacConfig)
    node.mac.declareMethod("ssc", uint8_t, cometos.MacConfig)
    node.mac.declareMethod("gs", cometos.MacStats)
    node.mac.declareMethod("rs", None)
    
    node.addModule(remoteAccessModule, cometos.TrafficGen.MODULE_NAME, node.id)
    node.tg.declareMethod("run", bool, cometos.TgConfig)
    node.tg.declareMethod("reset", bool)
    node.tg.declareMethod("getResults", cometos.TgResults)
    node.tg.declareMethod("getRttStats", cometos.SumsMinMaxTs64)
    node.tg.declareMethod("gsc", cometos.StackCfg)
    node.tg.declareMethod("ssc", uint8_t, cometos.StackCfg)
    node.tg.declareMethod("rsc", uint8_t)
    node.tg.declareEvent("tf", cometos.TgResults)
    
    node.addModule(remoteAccessModule, "rm", node.id)
    node.rm.declareMethod("add", None, cometos.RouteInfo)
    node.rm.declareMethod("clear", None)
    node.rm.declareMethod("sr", bool, cometos.RouteInfo, uint8_t)
    node.rm.declareMethod("cs", bool)
    node.rm.declareMethod("gr", cometos.RouteInfo, uint8_t)
    node.rm.declareMethod("gnr", uint8_t)
    
    node.addModule(remoteAccessModule, "sys", node.id)
    node.sys.declareMethod("ping", uint16_t)
    node.sys.declareMethod("led", None, uint8_t)
    node.sys.declareMethod("run", None, uint16_t)
    node.sys.declareEvent("hb", uint16_t)
    node.sys.declareMethod("vers", uint16_t)    
    node.sys.declareMethod("ts", uint32_t)
    node.sys.declareMethod("ar", None)
    node.sys.declareMethod("ai", cometos.AssertShortInfo)
    node.sys.declareMethod("ac", None)
    node.sys.declareMethod("at", None, cometos.AssertShortInfo)
    node.sys.declareMethod("util", cometos.CoreUtilization)
    node.sys.declareMethod("fwv", uint64_t)
    node.sys.declareMethodAsync("taD", uint16_t, asyncT, "ta", None, uint16_t)
    node.sys.declareEvent("bar", cometos.AssertShortInfo)
    node.sys.bar.register(sysMon.booted)

    node.addModule(remoteAccessModule, cometos.TimeSyncService.MODULE_NAME, node.id)
    node.tss.declareMethod("start", bool, cometos.RoleCfg)
    node.tss.declareMethod("stop", bool)
    node.tss.declareMethod("gnt", cometos.TimeSyncData)
    
    node.addModule(remoteAccessModule, "otap", node.id)
    node.otap.declareMethod("gnmv", uint8_t)
    node.otap.declareMethod("gmv", cometos.SegBitVector, uint8_t)
    node.otap.declareMethod("si", None, uint16_t)
    node.otap.declareMethod("run", uint8_t, uint8_t, uint16_t)

    node.addModule(remoteAccessModule, "tm", node.id)
    node.tm.declareMethod("start", bool, cometos.TmConfig)
    node.tm.declareMethod("stop", bool)
    node.tm.declareMethod("gi", SumsMinMaxRssi32, cometos.TMNodeId)
    node.tm.declareMethod("gnl", NodeListX)
    node.tm.declareMethod("gns", uint32_t)
    node.tm.declareMethod("gnnm", uint32_t)
    node.tm.declareMethod("clear", bool)
    
    node.addModule(remoteAccessModule, "txp", node.id)
    node.txp.declareMethod("spl", uint8_t, cometos.TxPower)
    node.txp.declareMethod("gpl", cometos.TxPower)
    node.txp.declareMethod("rpl", uint8_t)
    
    node.addModule(remoteAccessModule, IPFWD_MODULE_NAME, node.id)
    node.ip.declareMethod("gc", cometos.IpConfig)
    node.ip.declareMethod("sc", uint8_t, cometos.IpConfig)
    node.ip.declareMethod("rc", uint8_t)
    node.ip.declareMethod("gac", cometos.IpConfig)
    
    node.addModule(remoteAccessModule, LOWPAN_MODULE_NAME, node.id)
    node.low.declareMethod("gs", cometos.LowpanAdaptionLayerStats)
    node.low.declareMethod("rs", None)
    node.low.declareMethod("sc", uint8_t, cometos.LowpanConfig)
    node.low.declareMethod("gc", cometos.LowpanConfig)
    node.low.declareMethod("rc", uint8_t)
    node.low.declareMethod("gac", cometos.LowpanConfig)
    
    node.addModule(remoteAccessModule, RELIABILITY_MODULE_NAME, node.id)
    node.srl.declareMethod("setCfg", bool, cometos.SrlConfig)
    node.srl.declareMethod("getCfg", cometos.SrlConfig)
    
    node.addModule(remoteAccessModule, "fm", node.id)
    node.fm.declareMethod("form", uint8_t)
    node.fm.declareMethod("ls", uint8_t, cometos.AirString)
    node.fm.declareMethod("rand", uint8_t, cometos.AirString, uint16_t, bool)
    node.fm.declareMethod("cat", uint8_t, cometos.AirString)
    node.fm.declareMethod("hex", uint8_t, cometos.AirString)
    node.fm.declareMethod("rm", uint8_t, cometos.AirString)
    node.fm.declareMethod("rcp", None, uint16_t, cometos.AirString)
    
    node.addModule(remoteAccessModule, "fsf", node.id)
    node.fsf.declareMethod("f", uint8_t)
    
    node.addModule(remoteAccessModule, "loof", node.id)
    node.loof.declareMethod("gs", cometos.LowpanOrderedForwardingStats)
    node.loof.declareMethod("rs", None)
    node.loof.declareMethod("gc", cometos.LowpanOrderedForwardingCfg)
    node.loof.declareMethod("sc", uint8_t, cometos.LowpanOrderedForwardingCfg)
    node.loof.declareMethod("gac", cometos.LowpanOrderedForwardingCfg)
    node.loof.declareMethod("rc", uint8_t)
    
    node.addModule(remoteAccessModule, "udp", node.id)
    node.udp.declareMethod("gc", cometos.UdpConfig)
    node.udp.declareMethod("rc", None)
    node.udp.declareMethod("sc", uint8_t, UdpConfig)
    node.udp.declareMethod("gac", UdpConfig)
    return node


# get node information and static routes from xml file
from xml.etree.ElementTree import ElementTree
tree = ElementTree()
tree.parse(networkFile)
xmlNodes = list(tree.iter("node")) 
nodes = []
nIds = list()

print "Setting up Remote Modules"
for xmlNode in xmlNodes:
    currId = int(xmlNode.attrib["id"], 0)
    # only process node if it is not the bs node!
    if currId != bs.id:
        currNode = initNode(currId, remote)
        nIds.append(currNode.id)
        nodes.append(currNode)
        currNode.led = int(xmlNode.attrib["ledset"], 0)
        currNode.rtEntries = list(xmlNode.iter("rtentry"))
    else:
        bs.rm.add = bs.rm.addRoute
        bs.rm.clear = bs.rm.clearRouting
        bs.rm.clear()
        bs.led = int(xmlNode.attrib["ledset"], 0)
        bs.rtEntries = list(xmlNode.iter("rtentry"))
        setNode(bs, bs.led, bs.rtEntries, False)

ids()

gw = initNode(0xFFFF, raGateway)


tstcfg = TgConfig(1000, 100, False, 0, True, 0, 256, IM_POISSON, 5, 2000)

asi = cometos.AssertShortInfo()
asi.wdtReset = True
asi.resetOffset = 3000
    

print "otaplist:"
otaplist = []    
for n in range(0, len(nodes)):
        otaplist.append(n)
        print hex(nodes[n].id)
print ""

#t = Topology("tm", nodes)
t = AvrTopology("tm", nodes + [bs])


cometos.run()

asMasterCfg = RoleCfg(True)
asSlaveCfg = RoleCfg(False)

def test(i):
    print str(nodes[i].tg.tf.subscribe(tftest, 1))
    print str(nodes[i].tg.run(tstcfg))

def checkMem(times):
	global nodes
	for i in range(times):
		print "==============="
		time.sleep(2.0)
		for n in nodes:
			u = n.sys.util()
			if u != None:
				print hex(n.id) + ":" + str(u.mem)
			time.sleep(0.5)
	
def tftest(id, retVal):
    print "Received finished from " + hex(int(id)) + "\n" + lib.getAttrString(retVal.udpStats)  + "\n" + lib.getAttrString(retVal.macStats)


def getPower():
    for n in nodes + [bs]:
        print n.txp.gpl().pwrLvl

def setPower(value):
    for n in nodes + [bs]:
        print n.txp.spl(value)
        

def restartNetwork():
	for n in nodes:
		n.sys.at(asi)

from modules.PyTrafficGen import PyIPv6Address
 
def checkPing(times):
    count = 0
    total = 0
    avg = Statistics()
    for i in range(times):
        for n in nodes:
            total = total + 1
            res = lib.benchmark(n.sys.ping)
            avg.add(res[1])
            print "run: " + str(i) + " node:" + hex(n.id) + " rtt:" + str(res)
            if res[1] > 1.0:
                count = count + 1
    
    print "Ping result: count=" + str(count) + " total=" + str(total) + " avg=" + str(avg.getAvg())
    return (count, total, avg)
 
import datetime
 
def cfsFormat(nodes):
    oldwt = remote.waitingTime
    remote.setWaitingTime(30)
    for n in nodes:
        print "Formatting cfs at {0}...".format(n.id),
        result = n.fsf.f()
        print " ok({0})".format(result)
    remote.setWaitingTime(oldwt)

def showVersion():
    for n in nodes:
        version = n.sys.fwv()
        print str(n.id) + ": " + str(version & 0xFFFF) + "|" + datetime.datetime.fromtimestamp(version>>16).strftime("%Y-%m-%d %H:%M:%S")
 

def printAllRoutes():
    for n in nodes:
        print ""
        print "Node " + hex(n.id)
        print "=============="
        for i in range(n.rm.gnr()):
            printRouteInfo(n.rm.gr(i))
            
 
def printRouteInfo(ri):
    nh = PyIPv6Address(ri.nextHop)
    prefix = PyIPv6Address(ri.prefix)
    preLen = ri.prefixLength
    
    print "prefix=" + str(prefix) + " len=" + str(preLen) + " nextHop=" + str(nh);

def getRetryCount():
    for n in nodes:
        print "node=" + str(n.id) 
        a=n.mac.gs()
        for i in range(8):
            print str(deref(a.retryCounter.get(i)))
        print ""

def getMacCfg(nodelist):
    for n in nodelist:
        mc = n.mac.gsc()
        print "MacConfig for node {0:x}".format(n.id)
        print "-------------------------"
        print "pers={0}|eqToActive={1}|txp={2}|chan={3}|minbe={4}|maxbe={5}\nmaxretr={6}|maxboretr={7}|mmode={8}|prommode={9}|nwkId={10}"\
                .format(mc.isPersistent(), mc.isEqualToActiveConfiguration(), mc.txPower, mc.channel, mc.minBE, mc.maxBE, mc.maxFrameRetries, mc.maxBackoffRetries, mc.macMode, mc.promiscuousMode, mc.nwkId)
        print ""

def listTime(duration):
    startTime = getTimeMs();
    startTimePal = palLocalTime_get()
    while getTimeMs() - startTime < duration:
        print str(palLocalTime_get()-startTimePal) + " " + str(getTimeMs()-startTime)
        
