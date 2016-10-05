# import cometos system
import swig.cometos as cometos
from threading import Thread
from RemoteAccess import *
import time;

class RecvDummy(cometos.EndpointWrap):   
    def initialize(self):
        pass
    def timeout(self):
          pass
    def recvIndication(self, frame, src, dst):
        #print "RECV MESSAGE from ",src," to ", dst, " count ",vectorLength(frame) 
        pass
    def recvConfirm(self, success):
        pass



# INSTANTIATION ---------------------------------------


#comm = cometos.TcpComm()
comm = cometos.TcpComm()

macDisp = cometos.Dispatcher4()
dummy1 = RecvDummy()
routing = Routing()
nwkDisp = cometos.Dispatcher2()
dummy2 = RecvDummy()
remote = RemoteAccess() 
ca_ = CommAssessment()

#printf=cometos.PrintfApp()


# WIRING ----------------------------------------------

comm.gateIndOut.connectTo(macDisp.gateIndIn);
macDisp.gateReqOut.connectTo(comm.gateReqIn);

macDisp.gateIndOut.get(0).connectTo(routing.gateIndIn);
routing.gateReqOut.connectTo(macDisp.gateReqIn.get(0));

macDisp.gateIndOut.get(1).connectTo(dummy1.gateIndIn);
dummy1.gateReqOut.connectTo(macDisp.gateReqIn.get(1));

macDisp.gateIndOut.get(2).connectTo(ca_.gateIndIn);
ca_.gateReqOut.connectTo(macDisp.gateReqIn.get(2));
    
routing.gateIndOut.connectTo(nwkDisp.gateIndIn);
nwkDisp.gateReqOut.connectTo(routing.gateReqIn);

nwkDisp.gateIndOut.get(0).connectTo(remote.gateIndIn);
remote.gateReqOut.connectTo(nwkDisp.gateReqIn.get(0));

nwkDisp.gateIndOut.get(1).connectTo(dummy2.gateIndIn);
dummy2.gateReqOut.connectTo(nwkDisp.gateReqIn.get(1));



       
# initialize CometOS
print "Initialize CometOS"
cometos.initialize()

comm.connect("localhost", 9001)
#comm.listen(9002)

node={}
ca={}
for x,y in [ (20,10), (1,1), (2,2),(3,3), (4,4), (5,5), (11,987), (12,33932), (13,34464)]:
    node[x] = RemoteModule(remote, "sys", y)
    node[x].declareMethod("ping", uint16_t)
#    node[x].declareMethod("util", cometos.CoreUtilization)
    node[x].declareMethod("led", None, uint8_t)
    
    ca[x] = RemoteModule(remote, "ca", y)
    ca[x].declareVariable("dest", uint16_t)
    ca[x].declareVariable("payl", uint8_t)
    ca[x].declareMethod("res", cometos.CommAssessmentResults)
    ca[x].declareMethod("rtt", None,uint16_t) 



print "Run CometOS"


def run():
    while cometos.run_once()==False:
        pass
    
cometosThread = Thread(target=run)
cometosThread.start()


def stop():
    cometos.stop() 
    cometosThread.join()
    exit()
    
    
def benchmark(fun):
    start=time.time()
    fun()
    return (time.time()-start)

def test():
    for y in range(1000):
        for x in range(6):
            res={}
            def pinging():
                res[0]=node[x].ping()
            print "pinging ",x," in ",benchmark(pinging)," (",res[0],")"



