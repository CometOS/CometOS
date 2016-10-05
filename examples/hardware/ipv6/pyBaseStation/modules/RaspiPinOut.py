import subprocess
import socket
from threading import Timer



class RaspiPinOut():
    def __init__(self, offsetToStartSec, offsetToEndSec, targetAddr):
        self.startTimer = Timer(offsetToStartSec, self.switchState, 1)
        self.endTimer = Timer(offsetToEndSec, self.finished, 0)
        self.targetAddr = targetAddr
        
        
    def connect(self): 
        try:
            self.s = socket.create_connection(targetAddr)
            print "Socket connection to {0}:{1} success".format(self.targetAddr[0], self.targetAddr[1])
        except: 
            self.s = None
            print "Socket connection to {0}:{1} failed".format(self.targetAddr[0], self.targetAddr[1])
    
    
    def switchState(self, arg):
        # subprocess.call(["gpio", "-g", "write", "17", str(1)])
        subprocess.call(["echo", "switchState with " + str(arg) + " called"])
        if self.s == None:
            self.connect()
            
        if self.s != None:
            self.s.send(str(arg))
            self.s.close()
                    
    
    def startEvent(self, eventType):
        if eventType == "START":
            print "starting timers" 
            startTimer.start()
            endTimer.start()
        else:
            print "eventType " + eventType + " not recognized"
            