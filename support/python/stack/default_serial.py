import swig.cometos as cometos
import cometosMisc as lib
from RemoteAccess import *
from PyOtap import *
from PyAssociationService import *
import sys

# CometOS default stack (Python)
#
#                 RemoteAccess
#                      |
#    open   OTAP  RealibilityLayer  	PrintApp    AssociationService
#     |0_____|1________|2________________|3_________________|4
#                       |
#                 Dispatcher<5>
#                       |
#  	    open       SimpleRouting  TopologyMonitor
#        |0_____________|1_____________|2
#                |
#           Dispatcher<4>
#                |
#             SerialComm

comm = cometos.SerialComm(sys.argv[1]) 
print "initializing serial comm at " + sys.argv[1]
macDisp = cometos.Dispatcher4()
dummyMac = lib.RecvDummy()
routing = cometos.SimpleRouting()
nwkDisp = cometos.Dispatcher8()
dummyNwk = lib.RecvDummy()
otap = PyOtap()
reliability=cometos.SimpleReliabilityLayer("srl")
remote = RemoteAccess() 
printf=cometos.PrintfAppReceiver()
association=PyAssociationService()
topo=TopologyMonitorX()

comm.gateIndOut.connectTo(macDisp.gateIndIn);
macDisp.gateReqOut.connectTo(comm.gateReqIn);

macDisp.gateIndOut.get(0).connectTo(dummyMac.gateIndIn);
dummyMac.gateReqOut.connectTo(macDisp.gateReqIn.get(0));

macDisp.gateIndOut.get(1).connectTo(routing.gateIndIn);
routing.gateReqOut.connectTo(macDisp.gateReqIn.get(1));

macDisp.gateIndOut.get(2).connectTo(topo.gateIndIn);
topo.gateReqOut.connectTo(macDisp.gateReqIn.get(2))

routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

nwkDisp.gateIndOut.get(0).connectTo(dummyNwk.gateIndIn);
dummyNwk.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));

nwkDisp.gateIndOut.get(1).connectTo(otap.gateIndIn);
otap.gateReqOut.connectTo(nwkDisp.gateReqIn.get(1));

nwkDisp.gateIndOut.get(2).connectTo(reliability.gateIndIn);
reliability.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));

nwkDisp.gateIndOut.get(3).connectTo(printf.gateIndIn);
printf.gateReqOut.connectTo(nwkDisp.gateReqIn.get(3));

nwkDisp.gateIndOut.get(4).connectTo(association.gateIndIn);
association.gateReqOut.connectTo(nwkDisp.gateReqIn.get(4));

reliability.gateIndOut.connectTo(remote.gateIndIn);
remote.gateReqOut.connectTo(reliability.gateReqIn);

remote.setWaitingTime(3)
