import swig.cometos as cometos
import time

class PyAssociationService(cometos.EndpointWrap):  
    def __init__ (self):
        cometos.EndpointWrap.__init__(self)
        self.nodes = {}  
        self.observers = []      
    def initialize(self):
        pass
    
    def timeout(self):
        pass
    
    def recvIndication(self, frame, src, dst):
        if (self.nodes.has_key(src)==False):
            for observer in self.observers:
                observer(src)   
        self.nodes[src] = time.time()
    def recvConfirm(self, success):
        pass
    
    def addObserver(self, callback):
        self.observers.append(callback)
        
    def getNodes(self, age=3600.0):
        '''
        returns a list of nodes which last sign of life 
        is smaller than the age value. 
        '''
        ret = []
        for id, timestamp in self.nodes.items(): 
            if ((time.time() - timestamp)<age):
                ret.append(id)
        return ret
        
    
    def getAges(self):
        '''
        returns dictionary including all node and their ages.
        ''' 
        ret = {}
        for id, timestamp in self.nodes.items(): 
            ret[id] = time.time() - timestamp
        return ret
