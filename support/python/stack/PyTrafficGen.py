from swig.cometos import *
import cometosMisc as lib
import sys
import traceback

class PyIPv6Address():
    def __init__(self, addr):
        self.addr = addr;

    
    def getShortAddr(self):
        return int(self.addr.getAddressPart(7))
        
    def __str__(self):
        s = ""
        for i in range(8):
            s = s + "%x" % self.addr.getAddressPart(i)
            if (i < 7):
                s = s + ":"
        return s

    def __eq__(self, other):
        for i in range(8):
            if self.addr.getAddressPart(i) != other.addr.getAddressPart(i):
                return False
        
        return True
    
    def __hash__(self):
        ''' Hash function based on the least significant 16 bit of the IPv6 address '''
        return int(self.addr.getAddressPart(7))
    


    


class PyTrafficGen(TrafficGen):
    def __init__(self, name, tss=None):
        TrafficGen.__init__(self, name)
        self.udpListeners = []
        self.eventListeners = {}
        self.stats = {}
        self.tss = tss
    
    def registerUdpListener(self, handler):
        self.udpListeners.append(handler)
        return True
        
    def deregisterUdpListener(self, handler):
        self.udpListeners.remove(handler)
        return True
    
    def registerEventListener(self, name, handler):
        if not name in self.eventListeners:
            self.eventListeners[name] = []
        
        self.eventListeners[name].append(handler)
        return True
        
    def deregisterEventListener(self, name, handler):
        assert name in self.eventListeners
        self.eventListeners[name].remove(handler)
        return True
    
    
    def transferFinished(self, id, obj):
        for fun in self.finishListeners:
            fun(str(id), obj)
    
    def handleIncomming(self, ipAddr, srcPort, dstPort, data, len):
        try:
            localTs = NetworkTime.get()
#             if self.tss != None:
#                 localTs = self.tss.getMasterTime(palLocalTime_get())
#             else:
#                 localTs = palLocalTime_get()
            src = PyIPv6Address(ipAddr)
            pyData = uint8Array.frompointer(data)
            ts = (pyData[2] << 24) + (pyData[3] << 16) + (pyData[4] << 8) + pyData[5]   
            seq = (pyData[0] << 8) + pyData[1]
            
            print str(localTs) + ":received from " + str(src) + "; seq=" + str(seq) + "|ts=" + str(ts) + "|localTs=" + str(localTs) + "|lat=" + str(localTs-ts) # + "|array:" + str(pyData) + "(" + str(pyData.thisown) + ") "  
            for fun in self.udpListeners:
                fun(src.getShortAddr(), seq, localTs, ts)
        except:
            print "Error in PyTrafficGen:"
            traceback.print_exc()
            
        
        
        return;

