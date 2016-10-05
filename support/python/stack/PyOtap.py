from swig.cometos import *
from threading import Event


__docformat__ = "javadoc"

class PyOtapControl():
	def __init__(self):
		pass
	
	def otapTaskDone(self, event):
		pass


class PyOtap(PyOtapBlockTransfer):   
    '''Module for accessing OTAP Transport Protocol
    It is recommend to call ONLY sendSegmentBlocking
    '''
     
    def __init__(self):
        
        PyOtapBlockTransfer.__init__(self,"PyOtap")
        
        # allocate memory for OTAP
        self.MAX_SEGMENT_SIZE=1024
        self.data=uint8Array(self.MAX_SEGMENT_SIZE)
        
        self.event=Event()

    def sendDone(self,seg):
        self.event.set()

    def sendSegmentBlocking(self,dest, segment, segId):
        ''' Sends segment to target node, blocks until data is sent
            @param dest       destination address
            @param segment    iteratable container with segment bytes
            @param segId      segment id
        '''
        assert (len(segment)<=self.MAX_SEGMENT_SIZE)
        
        for i in xrange(len(segment)):
            self.data[i]=segment[i]
                
        self.sendSegment(dest, self.data, segId)
        self.event.wait()
        self.event.clear()

