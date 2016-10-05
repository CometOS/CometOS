from swig.cometos import *
from threading import Thread, Event

import time
from Queue import Queue


# TODO instead of NWK LAyer we only send node ID and length field
# We have to add and CRC error detection layer


#in seconds
SERIAL_TIMEOUT= 0.005

SERIAL_QUEUE_SIZE= 3

# in milliseconds
SERIAL_PACKET_POLL= 1



def cometosSerial(port, baudrate, queueIn, queueOut, stopSignal):
    __doc__='''
    Reads/Writes serial data. TODOD: current implementation tries
    to receive byte on after another, change this
    '''
    
    import serial
    ser = serial.Serial(port, baudrate, timeout=SERIAL_TIMEOUT)

    while not stopSignal.is_set():
        # try reading start byte, afterwards try reading packet data
        length = ser.read(1)  
        if len(length)>0:
            length=ord(length)
            frame= AirframeData()
            
            if length > 110:
                print "length > 110"
                ser.read(length)
            else:         
                while length>0 :
                    data = ser.read(length)
                    # timeout occurs and no data received
                    if len(data) == 0:
                        break  
                    length-=len(data)
                    for x in data:
                        vectorPush(frame, ord(x))
                # store frame if successfully received
                if length==0:
                    queueIn.put(frame)            
            
        # try sending all data/ one packet, make break after sending data?
               
        # Send next packet
        if not queueOut.empty():
            frame = queueOut.get()
            ser.write(chr(vectorLength(frame)))
            for i in range (vectorLength(frame)):
                ser.write(chr(vectorGet(frame, i)))
                                 
               
            
    # TODO close serial
    print "stop serial reader on port ", port
        
        

class SerialComm(LowerEndpointWrap):  
    __doc__ = ''' 
    Serial Communication interface for CometOS. 
    This class creates a separate thread for reading and writing 
    data from/ to a serial port. It is thread-safe. Currently,
    the main thread (in which the CometOS scheduler runs) polls
    all SERIAL_PACKET_POLL milliseconds for new packets in the
    income queue. Note that multiple instances (for different ports)
    may exist. During the reception of data, the maximal time between
    tow bytes can be SERIAL_TIMEOUT seconds. Note that after 
    sending a packet, the layer tries to receive a packet. Since 
    the timeout for reception is SERIAL_TIMEOUT, this also determines
    the interframe spacing between packets. Also this layer doesn't 
    check whether a received frame is valid. Use it together with
    a security layer to ensure validity of data (e.g CrcLayer).
    This protocol adds a Network Layer (may be changed in later releases).
    '''
    def __init__(self, port, baudrate):
        LowerEndpointWrap.__init__(self)
        self.seq = 0
        self.queueIn = Queue(SERIAL_QUEUE_SIZE)
        self.queueOut = Queue(SERIAL_QUEUE_SIZE)
        self.stopSignal = Event()
        self.thread = Thread(target=cometosSerial, args=(port, baudrate, self.queueIn, self.queueOut, self.stopSignal))
    

    def initialize(self):
        self.setTimer(SERIAL_TIMEOUT)
        self.thread.start()    
       
    # TODO: We need to implement some automatic way for closing serial ports
    def close(self):
        self.stopSignal.set()
        self.thread.join()
        
    
    def timeout(self):
        # check for new packets in queue and extract MAC header
        while not self.queueIn.empty():
            frame = self.queueIn.get()
            
            if vectorLength(frame)<6:
                continue
            header = NwkHeader()
            unserialize(frame, header)
            self.sendIndication(frame, header.src, header.dst)      
        self.setTimer(SERIAL_PACKET_POLL)
        
    
    def recvRequest(self, frame, dst):
        header = NwkHeader()
        header.src = 0
        header.dst = dst
        header.seq = self.seq
        self.seq += 1
        serialize(frame, header)
        if self.queueOut.full():
            print "warning, output queue full, discard packet"
            return False
        else:
            self.queueOut.put(frame)
            return True
    


