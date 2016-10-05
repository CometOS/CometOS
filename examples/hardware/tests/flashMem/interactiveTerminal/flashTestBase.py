#------------------------------------------------------------------------------

import sys
import time;
from threading import Thread

# add folder of CometOS's python scripts to search path
sys.path.append('../../../../src/python')

# import CometOS
import swig.cometos as cometos

if len(sys.argv) != 2:
    print "usage: " + sys.argv[0] + " [<comport>]"
    print "if <comport> is provided, direct SerialComm is used, otherwise a tcp connection is established to port 20000"
    os._exit(1)


import stack.default_serial as stack

# load CometOS helper classes
import cometosMisc as lib
from cometosMisc import *

# load classes for RMI support
import stack.RemoteAccess as ra

# load CometOS data types
from stack.RemoteAccess import uint32_t, uint16_t, uint8_t

from stack.PyOtap import *

from stack.PySysMon import PySysMon
logName =  "./bootlogs/" + time.strftime("%Y-%m-%d") + ".dat"
sysMon = PySysMon(logName)

#------------------------------------------------------------------------------
  
ex=lib.Script(['./scripts'], globals())
      
# initialize CometOS
cometos.initialize()

    
#------------------------------------------------------------------------------

def asyncT(node, counterValue):
	print "Received deferred ping event from " + hex(node) + ", counter=" + str(counterValue)    

def prettyPrintVec(vec):
    ''' expects cometos.FlashVector
    '''
    for i in range(16):
        print "%4d" % i,
    print ""
    for i in range(vec.getSize()):
        if i % 16 == 0 and i != 0:
            print ""
        print "0x%02x" % vec.get(i),
    print ""
    print ""
    

def write(node, addr, string):
    if (len(string) > FLASH_DATA_MAX_SIZE or (not node in nodes)):
        return False;
    else:
        print "Writing to device at address 0x%x" % addr
        cfg = S25FlWriteMsg(addr, string+"\0");
        prettyPrintVec(cfg.data)
        return nodes[node].ft.fw(cfg)
    
    

def read(node, addr, size):
    if (size > FLASH_DATA_MAX_SIZE or (not node in nodes)):
        return None;
    else:
        vec = nodes[node].ft.fr(cometos.S25FlAccessMsg(addr, size))
        print "Read data from address 0x%x:" % addr
        prettyPrintVec(vec)
        return vec

def erase(node, addr):
    if not (node in nodes):
        return False;
    else:
        return nodes[node].ft.fe(cometos.S25FlAccessMsg(addr, 0))
    

def dumpMem(node, addrStart, addrEnd, filename):
    curr = addrStart
    fh = open(filename, "wb")
    bytearr = []
    while (curr != addrEnd):
        toRead = addrEnd - curr
        if toRead > FLASH_DATA_MAX_SIZE:
            toRead = FLASH_DATA_MAX_SIZE
        data = read(node, curr, toRead)
        curr += toRead    
        for i in range(data.getSize()):
            bytearr.append(data.get(i))
    
    fh.write(bytearray(bytearr))


nodes={}
def addNode(id):
    obj=Node()
    #obj.id=id
    obj.x=0
    obj.y=0 
    nodes[id]=obj
    nodes[id].id=id
    nodes[id].addModule(stack.remote, "sys", id)
    nodes[id].sys.declareMethod("ping", uint16_t)
    nodes[id].sys.declareMethod("led", None, uint8_t)
    nodes[id].sys.declareMethod("run", None, uint16_t)
    nodes[id].sys.declareEvent("hb", uint16_t)
    nodes[id].sys.declareMethod("vers", uint16_t)    
    nodes[id].sys.declareMethod("ts", uint32_t)
    nodes[id].sys.declareMethod("ar", None)
    nodes[id].sys.declareMethod("ai", cometos.AssertShortInfo)
    nodes[id].sys.declareMethod("ac", None)
    nodes[id].sys.declareMethod("at", None, cometos.AssertShortInfo)
    nodes[id].sys.declareMethod("util", cometos.CoreUtilization)
    nodes[id].sys.declareMethodAsync("taD", uint16_t, asyncT, "ta", None, uint16_t)
    nodes[id].sys.declareEvent("bar", cometos.AssertShortInfo)
    nodes[id].sys.bar.register(sysMon.booted)
    
    nodes[id].addModule(stack.remote, "t2", id)
    nodes[id].t2.declareMethod("get",cometos.TrafficData, uint8_t, uint8_t)
        
   
    nodes[id].addModule(stack.remote, "tm", id)
    
    nodes[id].addModule(stack.remote, "ft", id)
    nodes[id].ft.declareMethod("fr", cometos.FlashVector, cometos.S25FlAccessMsg)
    nodes[id].ft.declareMethod("fw", bool, cometos.S25FlWriteMsg)
    nodes[id].ft.declareMethod("fe", bool, cometos.S25FlAccessMsg)
    
stack.association.addObserver(addNode)
#------------------------------------------------------------------------------
addNode(263)
r1 = cometos.S25FlAccessMsg(0, 40)

cometos.run()


