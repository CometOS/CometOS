#------------------------------------------------------------------------------

import sys
import time;
from threading import Thread

# add folder of CometOS's python scripts to search path
sys.path.append('../../src/python')

import swig.cometos as cometos

# import default CometOS communication stack 
# this is coincident with stack used in examples/node 
import stack.default_serial as stack

# load CometOS helper classes
import cometosMisc as lib

# load classes for RMI support
import stack.RemoteAccess as ra

# load CometOS data types
from stack.RemoteAccess import uint32_t, uint16_t, uint8_t

#------------------------------------------------------------------------------

# create and connect additional timesync module
tss = cometos.TimeSyncService("tss", False, 1)
tsDisp = cometos.Dispatcher2()

tss.gateInitialOut.connectTo(tsDisp.gateReqIn.get(0));
tsDisp.gateIndOut.get(0).connectTo(tss.gateInitialIn);

tss.gateTimestampOut.connectTo(tsDisp.gateReqIn.get(1));
tsDisp.gateIndOut.get(1).connectTo(tss.gateTimestampIn);

stack.macDisp.gateIndOut.get(0).connectTo(tsDisp.gateIndIn);
tsDisp.gateReqOut.connectTo(stack.macDisp.gateReqIn.get(0));

ex=lib.Script(['./scripts'], globals())
      
# initialize CometOS
cometos.initialize()

    
#stack.comm.listen(20000)
#stack.comm.connect("localhost",20000)

#------------------------------------------------------------------------------
# build up node dictionary using association service 
class Node(object):
    pass
    
node={}
def addNode(id):
    obj=Node()
    #obj.id=id
    obj.x=0
    obj.y=0 
    node[id]=obj
    node[id].sys = ra.RemoteModule(stack.remote, "sys", id)
    node[id].sys.declareMethod("ping", uint16_t)
    node[id].sys.declareMethod("led", None, uint8_t)
    node[id].sys.declareMethod("run", None,uint16_t)
    node[id].sys.declareEvent("hb", uint16_t)
    node[id].traffic = ra.RemoteModule(stack.remote, "t2", id)
    node[id].traffic.declareMethod("get",cometos.TrafficData, uint8_t, uint8_t)
    node[id].otap = ra.RemoteModule(stack.remote, "otap", id)
    node[id].otap.declareMethod("init", uint8_t, uint8_t,uint16_t)
    node[id].otap.declareMethod("miss",cometos.SegBitVector)
    node[id].otap.declareMethod("veri", uint8_t, uint16_t, uint32_t)                                                               
    node[id].otap.declareMethod("run", uint8_t, uint8_t, uint16_t)
stack.association.addObserver(addNode)
#------------------------------------------------------------------------------


cometos.run()


