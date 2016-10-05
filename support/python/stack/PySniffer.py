from swig.cometos import *

class PySniffer(cometos.Sniffer):  
    def __init__ (self):
        cometos.Sniffer.__init__(self)
        self.handler=None
    def setHandler(self, handler):
        self.handler=handler
    def forward(self, frame, src):
        if handler!=None:
            self.handler(frame,src)  

