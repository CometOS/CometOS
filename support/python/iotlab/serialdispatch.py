import swig.cometos as cometos
import cometosMisc as lib
import traceback

class PySerialDispatch(cometos.SerialDispatch):
    
    def __init__(self, serialTunnel, frameTimeout=75):
        super(PySerialDispatch, self).__init__()
        self.serialTunnel = serialTunnel
        self.frameTimeout = frameTimeout
            
    
    def doInitializeGates(self):
        print "Initialization call from C++ module; connect to virtual com ports"
        try:
            for mapping in self.serialTunnel.portMappings:
                port = mapping["port"]
                uid = mapping["uid"]
                print "Create serial forward route for node {0} to port {1}".format(uid, port)
                self.createForwarding(port, uid, self.serialTunnel.baudrate, self.frameTimeout)
        except:
            traceback.print_exc() 
 
