import swig.cometos as cometos
import cometosMisc as lib
from RemoteAccess import *
from PyOtap import *
 

#          TCPComm
#             |
#          PySniffer
#             |
#         Dispatcher2
#       ______|_____
#      |0           |1
#   Routing      RecvDummy
#      |
#  Dispatcher2
#      |________________________
#      |0            |1         |2
#  RemoteAccess  RecvDummy    PyOtap

comm = cometos.TcpComm() 
#sniffer=lib.PySniffer() 
macDisp = cometos.Dispatcher2()
dummy1 = lib.RecvDummy()
routing = cometos.Routing()
nwkDisp = cometos.Dispatcher4()
dummy2 = lib.RecvDummy()
remote = RemoteAccess() 
otap = PyOtap()


comm.gateIndOut.connectTo(macDisp.gateIndIn);
macDisp.gateReqOut.connectTo(comm.gateReqIn);


#comm.gateIndOut.connectTo(sniffer.gateIndIn);
#sniffer.gateReqOut.connectTo(comm.gateReqIn);
    
#sniffer.gateIndOut.connectTo(macDisp.gateIndIn);
#macDisp.gateReqOut.connectTo(sniffer.gateReqIn);

macDisp.gateIndOut.get(0).connectTo(routing.gateIndIn);
routing.gateReqOut.connectTo(macDisp.gateReqIn.get(0));

macDisp.gateIndOut.get(1).connectTo(dummy1.gateIndIn);
dummy1.gateReqOut.connectTo(macDisp.gateReqIn.get(1));

routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

nwkDisp.gateIndOut.get(0).connectTo(remote.gateIndIn);
remote.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));

nwkDisp.gateIndOut.get(1).connectTo(dummy2.gateIndIn);
dummy2.gateReqOut.connectTo(nwkDisp.gateReqIn.get(1));

nwkDisp.gateIndOut.get(2).connectTo(otap.gateIndIn);
otap.gateReqOut.connectTo(nwkDisp.gateReqIn.get(2));
