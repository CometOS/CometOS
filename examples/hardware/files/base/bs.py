#------------------------------------------------------------------------------

import sys
import time;
import traceback
from threading import Thread


cometosPyPath='../../../../support/python'

# add folder of CometOS's python scripts to search path
sys.path.append(cometosPyPath)

# import CometOS
import swig.cometos as cometos
from stack.PyTopology import *

# TODO some nicer parameterization would be great
# import default CometOS communication stack 
# this is coincident with stack used in examples/node 
if len(sys.argv) == 1:
    import stack.default as stack
    #stack.comm.connect("localhost",20000)
    stack.comm.connect("134.28.77.248",1233)
elif len(sys.argv) == 2:
    import stack.default_serial as stack
else:
    print "usage: " + sys.argv[0] + " [<comport>]"
    print "if <comport> is provided, direct SerialComm is used, otherwise a tcp connection is established to port 20000"
    os._exit(1)


# load CometOS helper classes
import cometosMisc as lib

# load classes for RMI support
import stack.RemoteAccess as ra

# load CometOS data types
from stack.RemoteAccess import uint32_t, uint16_t, uint8_t

from stack.PyOtap import *

from stack.PySysMon import PySysMon
logName =  "./bootlogs/" + time.strftime("%Y-%m-%d") + ".dat"
sysMon = PySysMon(logName)

#------------------------------------------------------------------------------
  
ex=lib.Script(['./scripts',"{0}/script".format(cometosPyPath)], globals())
      
# initialize CometOS
cometos.initialize()

bs=Node()
bs.id = 0
bs.tm = stack.topo
bs.tm.gnl = bs.tm.getNodeList
bs.tm.gi = bs.tm.getInfo
bs.tm.gns = bs.tm.getNumSent
    
#------------------------------------------------------------------------------
otapCtrl = PyOtapControl()

stack.remote.setWaitingTime(5)
    
def asyncT(node, counterValue):
	print "Received deferred ping event from " + hex(node) + ", counter=" + str(counterValue)    

node={}
def addNode(id):
    try:
        obj=lib.Node()
        #obj.id=id
        obj.x=0
        obj.y=0 
        node[id]=obj
        node[id].id=id
        node[id].sys = ra.RemoteModule(stack.remote, "sys", id)
        node[id].sys.declareMethod("ping", uint16_t)
        node[id].sys.declareMethod("led", None, uint8_t)
        node[id].sys.declareMethod("run", None,uint16_t)
        node[id].sys.declareEvent("hb", uint16_t)
        node[id].sys.declareMethod("ai", cometos.AssertShortInfo)
        node[id].sys.declareMethod("ar", None)
        node[id].sys.declareMethod("ac", None)
        node[id].sys.declareMethod("at", None, cometos.AssertShortInfo)
        node[id].sys.declareMethod("create", None)
        node[id].sys.declareMethod("ma", None)
        node[id].sys.declareMethod("util", cometos.CoreUtilization)
        node[id].sys.declareMethodAsync("taD", uint16_t, asyncT, "ta", None, uint16_t)
        node[id].sys.declareEvent("bar", cometos.AssertShortInfo)
        node[id].sys.bar.register(sysMon.booted)
        
       
        node[id].tm = ra.RemoteModule(stack.remote, "tm", id)
        node[id].tm.declareMethod("start", bool, cometos.TmConfig)
        node[id].tm.declareMethod("stop", bool)
        node[id].tm.declareMethod("gi", SumsMinMaxRssi32, cometos.TMNodeId)
        node[id].tm.declareMethod("gnl", NodeListX)
        node[id].tm.declareMethod("gns", uint32_t)
        node[id].tm.declareMethod("gnnm", uint32_t)
        node[id].tm.declareMethod("clear", bool)
        
        node[id].txp = ra.RemoteModule(stack.remote, "txp" , id )
        node[id].txp.declareMethod("spl", uint8_t, cometos.TxPower)
        node[id].txp.declareMethod("gpl", cometos.TxPower)
        node[id].txp.declareMethod("rpl", uint8_t)
        
        
        node[id].traffic = ra.RemoteModule(stack.remote, "t2", id)
        node[id].traffic.declareMethod("get",cometos.TrafficData, uint8_t, uint8_t)
        
        node[id].addModule(stack.remote, "otap", node[id].id)
    #     node[id].otap = ra.RemoteModule(stack.remote, "otap", id)
        node[id].otap.declareMethod("gnmv", uint8_t)
        node[id].otap.declareMethod("gmv", cometos.SegBitVector, uint8_t)
        node[id].otap.declareMethod("si", None, uint16_t)
        node[id].otap.declareMethod("run", uint8_t, uint8_t, uint16_t)
    
        node[id].addModule(stack.remote, "fm", node[id].id)
        node[id].fm.declareMethod("ls", uint8_t, cometos.AirString)
        node[id].fm.declareMethod("rand", uint8_t, cometos.AirString, uint32_t, bool)
        node[id].fm.declareMethod("form", uint8_t)
        node[id].fm.declareMethod("cat", uint8_t, cometos.AirString)
        node[id].fm.declareMethod("hex", uint8_t, cometos.AirString)
        node[id].fm.declareMethod("rm", uint8_t, cometos.AirString)
        node[id].fm.declareMethod("rcp", None, uint32_t, cometos.AirString) # TODO node_t ??
        #node[id].fm.declareMethod("prop", uint8_t, cometos.AirString)
        #node[id].fm.declareMethodAsync("propD", cometos.FileProperties, propReceived, "prop", uint8_t, cometos.AirString)
	node[id].fm.declareMethod("msg", uint8_t, uint16_t, cometos.AirString)
	node[id].fm.declareMethod("example", uint8_t, cometos.AirString)
	node[id].fm.declareMethod("deluge", uint8_t, cometos.AirString)
        

        node[id].topo = ra.RemoteModule(stack.topo, "tm", id)
    except:
        print "Error adding node:"
        traceback.print_exc()
        
    

addNode(46708)
addNode(10326)
nodes=list(node.values())
t=AvrTopology("tm",nodes+[bs])

stack.association.addObserver(addNode) 

def setPwrLvl(pwrLvl, nodes):
    for n in nodes:
        n.txp.spl(TxPower(pwrLvl))
        
def topoTest(t, nodes, startPwr=255, minutes=10):
    pwr = startPwr
    while pwr > 0:
        setPwrLvl(pwr, nodes)
        t.clear()
        t.start(500,10)
        time.sleep(60*minutes)
        t.stop()
        t.getValues()
        filename= "pwr%d.raw" % pwr
        t.log(filename,False)
        pwr -= (255 / 16)
        

cometos.run()




