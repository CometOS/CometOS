from xml.etree.ElementTree import ElementTree
import os.path
import glob
import time
import os
import swig.cometos as cometos
from stack.RemoteAccess import *

startTime = 0

class redirect:
    def __init__(self, filename):
        dir = os.path.dirname(filename)
        if not os.path.exists(dir):
            print("Making dir " + dir + " ")
            os.makedirs(dir)
        if not os.path.isfile(filename):
            open(filename, 'w').close()
        self.cmdLog = open(filename, 'a')
        
    def __del__(self):
        if self.cmdLog:
            self.cmdLog.close()
        
    def write(self, string):
        self.cmdLog.write(string)
        self.cmdLog.flush()

class RecvDummy(cometos.EndpointWrap):   
    def initialize(self):
        pass
    def timeout(self):
        pass
    def recvIndication(self, frame, src, dst):
        print "RECV MESSAGE from ",src," to ", dst, " count ",vectorLength(frame) 
        pass
    def recvConfirm(self, success):
        pass   
     
           
class Script:
    """
    Manages the execution of scripts.
    Be aware that the execution of a script takes place in the passed scope.
    To run the script in the global scope use Script([...],globals())
    """
    
    def __init__(self, dirs, scope):      
        for dir in dirs:
            self.updateDir(dir)
            self.scope=scope
    
    def updateDir(self, dir):
        for file in glob.glob(dir+"/*.py"):
            self.declareScript(file)   
    
    def declareScript(self, file):
        print "declare script: "+file
        internal_name = "__" + os.path.splitext(os.path.basename(file))[0];
        if self.__dict__.has_key(internal_name):
            print "Error attribute with method name already exists"
            return
        # create function call, pass global scope and args to script
        self.__dict__[internal_name] = lambda *args: execfile(file, self.scope,{"args":args})
        

    def __getattr__(self, name):
        internal_name = "__" + name;

        if not self.__dict__.has_key(internal_name):
            print "Error attribute doesn't exist"            
            return
        else:
            return self.__dict__[internal_name]

 
        
# members are dynamically added during initialization
class Node(object):
    def addModule(self, remote, name, id):
        internal_name = "__" + name;
        if self.__dict__.has_key(internal_name):
            print "Error: Module name <" + name + "> already exists"
            return False
        if isinstance(remote, RemoteAccess):
            self.__dict__[internal_name] = RemoteModule(remote, name, id)
            return True
        else:
            print "Invalid remote module (has to be of type " + str(type(RemoteAccess))
            return False
        
         
    def __getattr__(self, name):
        internal_name = "__" + name;

        if not self.__dict__.has_key(internal_name):
#             print "Error attribute " + name + " doesn't exist"            
            return None
        else:
            return self.__dict__[internal_name]
    
    def __str__(self):
        return "Node({0})".format(self.id)
    def __repr__(self):
        return self.__str__()
    pass
         

def loadXmlTopology(topofile):
    doc = ElementTree(file=topofile)
    nodes={}
    idx=0
    for node in doc.findall('node'):
        obj=Node()
        obj.id=int(node.get('id'), 16)%65536
        obj.x=node.get('x')
        obj.y=node.get('y')  
        nodes[idx]=obj
        idx=1+idx
    return nodes


import os


def getTime():
    ''' Returns the current SYSTEM time in a unit of seconds, but as a 
    floating point number (meaning that the resolution can be higher). 
    The acutal resolution depends
    on what the underlying operating system provides. 
    
    NOTE that the time returned here is most likely NOT the same as what all 
    the underlying wrapped C++ modules use as a time base! 
    
    Use cometos.palLocalTime_get() to access a common local time base from within
    python scripts. 
    '''
    if os.name=='posix':
        return time.time()
    else:
        return time.clock()

def getTimeMs():
    return getTime() / 1000
    
def benchmark(fun, *args):
    start=getTime()
    res=fun(*args)
    return (res,(getTime()-start))


'''
Runs a simple time synchronization using the algorithm of Cristian. 
This method does not change the clock of the remote modules but rather 
returns timestamps corresponding to approximately the same time instant.

Estimates round-trip time between RemoteAccess modules, not considering the
time spent on different layers (that is, CCA/CSMA biases the results!)
@param remoteTimeFun remotely accessible function which returns a timestamp representing 
           the current system time of the remote device
@param localTimeFun function which returns the local time
@param tries number of round trips -- the tuple with the shortest rtt is used
@return: a tuple containing (tsLocal, tsRemote, accuracy)
         where tsLocal is the local time and tsRemote is the estimated system time of
         the remote device at tsLocal. Accuracy contains the maximum deviation between the
         two timestamps (that is, the remote system time might in reality be tsRemote +- accuracy)
''' 
def timeSync(remoteTimeFun, timeFunction, tries):
    print "Running simple time synchronization"
    t0 = timeFunction();
    rttMin = float("inf")
    tsLocal = 0
    tsRemote = 0
    rtt = rttMin
    for i in range(tries):
        time.sleep(0.2)
        t0 = timeFunction()
        ts = remoteTimeFun()
        t1 = timeFunction()
        rtt = t1 - t0
#        print "tsLocal=" + str((t0+t1)/2) + "|tsRemote=" + str(ts) + "|rtt=" + str(rtt)
        if rtt < rttMin:
            rttMin = rtt
            tsLocal = (t0 + t1) / 2 # assume ts was generated after half rtt
            tsRemote = ts
            
    print "result: tsLocal=" + str(tsLocal) + "|tsRemote=" + str(tsRemote) + "|rtt=" + str(rttMin/2)
    return TimeConversion(tsLocal, tsRemote, rttMin/2) 
        
    
'''
    Encapsulate basic functions for conversion between some 
    remote and a the local clock, given that it is initialized 
    with a local and a remote timestamp which correspond to the
    same moment and run at the same speed 
    and the a maximum deviation at that instant in time
'''
class TimeConversion():
    def __init__(self, tsLocal, tsRemote, uncertainty):
        self.tl = tsLocal
        self.tr = tsRemote
        self.delta = uncertainty
    
    '''
    Returns the time difference between t1 and t0 where
    t0r represents the remote time at t0 and t1l the local 
    time at t1
    '''
    def getDiffLR(self, t1l, t0r):
        return t1l - (self.tl + t0r - self.tr);
    
    def deltaL(self, t0l):
        return t0l - self.tl
    
    def deltaR(self, t0r):
        return t0r - self.tr


def getAttrString(obj, prefix=None):
    if prefix == None:
        prefix = ""
    # generator (iterator)
    it=(line for line in dir(obj) if line.startswith("__")==False and line.startswith("_")==False and line!="this" and line !="reset")
    s=""
    for x in it:
        value = str(obj.__getattribute__(x))
        if 'proxy of <Swig Object of type' in value or '<Swig Object of type' in value:
            # ignore anything which cannot be reasonably printed
            pass
        else:
			s= s+" "+prefix+x+"="+value 
    return s

    
    
def compareSimpleObj(obj, other):
    if type(obj) != type(other):
        return false
    # generator (iterator)
    it=(line for line in dir(obj) if line.startswith("__")==False and line!="this" and line !="reset")
    res = True
    for x in it:
        res = res and (obj.__getattribute__(x) and other.__getattribute__(x))
                        
    return res

    
