# import cometos system
import sys
sys.path.append('../../src/python')

import swig.cometos as cometos
from threading import Thread
from stack.RemoteAccess import *
import PacketAnalyzer2 as PacketAnalyzer
#import PacketAnalyzer

import time;
import sys;



if (len(sys.argv)==4):
    print "Start Forwarder in Client Mode"
elif (len(sys.argv)==3):
    print "Start Forwarder in Server Mode"  
else:
    print """
    Error: Wrong number of arguments
    
    Client-Mode: python forwarder.py <SERIAL_PORT> <IP_ADDRESS> <PORT>
    Server-Mode: python forwarder.py <SERIAL_PORT> <PORT>
    """
    exit()



class PySerialFwdBounce(cometos.SerialFwdBounce):   
    def __init__(self,name, interval,debug):
       SerialFwdBounce.__init__(self,name, interval,debug)  
        
    def recvPkt(self, frame, src):
        
        
        pkt=[]
        for i in range(cometos.vectorLength(frame)):
            pkt.append(int(cometos.vectorGet(frame, i)))

        PacketAnalyzer.recvPacket(str(src),pkt)
     




# INSTANTIATION ---------------------------------------
comm= cometos.SerialComm(sys.argv[1])
tcp = cometos.TcpComm() 
bounce= PySerialFwdBounce ("sb", 0, False);

 
  
# WIRING ----------------------------------------------
bounce.gateReqOut.connectTo(comm.gateReqIn);
comm.gateIndOut.connectTo(bounce.gateIndIn);

bounce.toTCP.connectTo(tcp.gateReqIn);
tcp.gateIndOut.connectTo(bounce.fromTCP);


# INIT & RUN ------------------------------------------   
# initialize 
print "Initialize CometOS"
initialize()
if (len(sys.argv)==3):
    tcp.listen(int(sys.argv[2]))
else:
    tcp.connect(sys.argv[2],int(sys.argv[3]))


   
    
print "Run CometOS"

def run():
    while cometos.run_once()==False:
        time.sleep(0.01)
        pass
    
cometosThread = Thread(target=run)
cometosThread.start()


PacketAnalyzer.run()

print "Stop CometOS"
cometos.stop() 
cometosThread.join()
exit()




