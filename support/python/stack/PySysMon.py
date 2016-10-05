import time
import os
import datetime
class PySysMon:
    def __init__(self, logfile=None):
        if logfile != None:
            logdir = os.path.dirname(logfile) 
            if not os.path.exists(logdir):
            	print "creating " + logdir
                os.makedirs(logdir)
            self.log = open(logfile, 'a')
            self.log.write("---- pyBase restart --- " + time.strftime("%Y%m%d") + "_" + time.strftime("%H%M%S") + " ----------------\n")
        else: 
            self.log = None
    
    def booted(self, id, asi):
        s = time.strftime("%Y-%m-%d_%H:%M:%S") + "|Id=" + hex(int(id))
        s += "|FileID=" + str(asi.fileId) + "|Line=" + str(asi.line) 
        s += "|wdtFlag=" + hex(asi.reset) + "|PC=" + hex(asi.pc)
        s += "|FwV=" + str(asi.fwVersion & 0xFFFF) + "|Fw_Ts=" + datetime.datetime.fromtimestamp(asi.fwVersion >> 16).strftime("%Y-%m-%d %H:%M:%S")
        print "Rcvd boot event: " + s
        s += "\n"
        if self.log != None:
            self.log.write(s)
            self.log.flush()
        

    def __del__(self):
        if not self.log.closed:
            self.log.close()
        