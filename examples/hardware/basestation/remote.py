import swig.cometos as cometos
from RemoteAccess import *
from SerialComm import *
from threading import Thread
import time


# Call this script with python -i ./remote.py to run interactive mode
# don't forget to call shutdown()


# INSTANTIATION ---------------------------------------

comm=SerialComm("COM44",57600) 
crc=Crc16Layer()
remote=RemoteAccess() 
disp=cometos.Dispatcher2()
#printf=cometos.PrintfApp()


# WIRING ----------------------------------------------

comm.gateIndOut.connectTo(crc.gateIndIn);
crc.gateIndOut.connectTo(disp.gateIndIn);
disp.gateIndOut.get(0).connectTo(remote.gateIndIn);
#disp.gateIndOut.get(1).connectTo(printf.gateIndIn);

#printf.gateReqOut.connectTo(disp.gateReqIn.get(1));
remote.gateReqOut.connectTo(disp.gateReqIn.get(0));
disp.gateReqOut.connectTo(crc.gateReqIn);
crc.gateReqOut.connectTo(comm.gateReqIn);
    
    
# create proxy for remote interface    
mod = RemoteModule(remote, "my", 0x127f)

mod.declareVariable("cnt", uint8_t)
mod.declareVariable("iv", uint16_t)
mod.declareMethod("led", None)
mod.declareMethod("sub", uint8_t, uint8_t, uint8_t)
mod.declareMethod("id", uint16_t)



# START -----------------------------------------------


# initialize CometOS
cometos.initialize()

# run CometOS in separate thread
cometosThread = Thread(target=cometos.run)
cometosThread.start()

# add some interactive stuff
#time.sleep(0.5)
#mod.iv = 99
#print "Get ID ", mod.id(), "  iv=",mod.iv 
#mod.led()
#time.sleep(1.5)


def shutdown():
    cometos.stop() 
    cometosThread.join()
    comm.close()
    exit()
    


# stop CometOS and wait for finish
#shutdown()