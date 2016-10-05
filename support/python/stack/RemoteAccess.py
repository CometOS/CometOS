from swig.cometos import *
from threading import Event, Lock
import cometosMisc as lib
import time

''' to free a sequence number which before got leaked (e.g. due to bit errors)'''
MAX_REMOTE_TIMEOUT_SEC = 60.0 

# maximum time waiting for a resposne from the remote node

class uint8_t:
    def __init__(self):
        self.a = 0
    
class uint16_t:
    def __init__(self):
        self.a = 0
    
class uint32_t:
    def __init__(self):
        self.a = 0

class uint64_t:
    def __init__(self):
        self.a = 0
        
remoteEventCallback={}
    
class RemoteVariable:
    def get(self):
        frame = AirframeData()
        serialize(frame, AirString(self.name))
        serializeU8(frame, 0) # type is remote variable
        resp =self.sendCommand(frame);

        if resp == None:
            print "Remote Access Failed"
            return None
        
        if unserializeU8(resp) == 1:
            print "no valid response"
            return None
        
        if self.remoteType == uint8_t:
            return unserializeU8(resp )
        elif self.remoteType == uint16_t:
            return unserializeU16(resp )
        else:
            value = self.remoteType();
            unserialize(resp , value);
            return value;

    
    def set(self, value):    
        frame = AirframeData()
        
        if self.remoteType == uint8_t:
            serializeU8(frame, value)
        elif self.remoteType == uint16_t:
            serializeU16(frame, value)
        else:
            serialize(frame, value);
    
        serialize(frame, AirString(self.name))
        serializeU8(frame, 0) # type is remote variable
        resp =self.sendCommand(frame);
        
        if resp == None:
            print "Remote Access Failed"
            
        if unserializeU8(resp) == 1:
            print "no valid response"
            return None
    
    def __init__(self,  sendCommand, name, remoteType):
        self.remoteType = remoteType
        self.name=name
        self.sendCommand = sendCommand

class RemoteEvent:
    def __init__(self, sendCommand, name,mod, retType,id):
        self.retType = retType
        self.name = str(name)
        self.mod=str(mod)
        self.id=str(id)
        self.sendCommand = sendCommand;
    
    def call(self, id, frame):
        try:
            if self.retType == uint8_t:
                val= unserializeU8(frame)
            elif self.retType == uint16_t:
                val= unserializeU16(frame)
            else:
                val=self.retType();
                unserialize(frame, val);
            self.callback(id,val)
        except Exception as e:
            print "Error: " + str(e.args)      
    
    def register(self, callback):
        global remoteEventCallback
        
        self.callback=callback
        
        if self.id not in remoteEventCallback:
            remoteEventCallback[self.id]={}
                      
        if self.mod not in remoteEventCallback[self.id]:
            remoteEventCallback[self.id][self.mod]={}
            
        remoteEventCallback[self.id][self.mod][self.name]= self
        
    def subscribe(self, callback, counter, local=None):
        self.register(callback)
        
        frame = AirframeData()
        serializeU16(frame, counter) 
        serialize(frame, AirString(self.name))
        serializeU8(frame, 2) # type is remote variable
        
        resp = self.sendCommand(frame)
        
        if resp == None:
            print "subscribe failed: timeout"
            return False
        
        if unserializeU8(resp) == 1:
            print "subscribe failed: remote error"
            return False
        else:
           return True
            
    def unsubscribe(self):
        frame = AirframeData()

        serialize(frame, AirString(self.name))
        serializeU8(frame,3) # type is remote variable
        
        resp = self.sendCommand(frame)
        
        if resp == None:
            print "unsubscribe failed: timeout"
            return None
        
        if unserializeU8(resp) == 1:
            print "unsubscribe failed: remote error"
        else:
            print "unsubscribe succeed"
        
        

class RemoteMethod:
    def __init__(self, sendCommand, name, retType, isAsync, *argTypes):
        self.retType = retType
        self.argTypes = argTypes
        self.name = name
        self.sendCommand = sendCommand;
        if isAsync: 
            self.type = 4 #RA_REMOTE_METHOD_EVENT
        else:
            self.type = 1 #RA_REMOTE_METHOD  TODO should be 
        
    def __call__(self, *args):
        if len(args) != len(self.argTypes):
            print "Wrong number of arguments; expected " + str(len(self.argTypes)) + " got " + str(len(args)) 
            return

        
        frame = AirframeData()
        
        for i in range(len(args)):
            if self.argTypes[i] == uint8_t:
                if  type(args[i]) != int : 
                    print "Error, Wrong Type U8 int expected"
                    return
                else:
                    serializeU8(frame, args[i]) 
            elif self.argTypes[i] == uint16_t:
                if  type(args[i]) != int : 
                    print "Error, Wrong Type U16 int expected"
                    return
                else:
                    serializeU16(frame, args[i]) 
            elif self.argTypes[i] == uint32_t:
                if  type(args[i]) != int : 
                    print "Error, Wrong Type U32 int expected"
                    return
                else:
                    serializeU32(frame, args[i]) 
            elif self.argTypes[i] == uint64_t:
                if  type(args[i]) != long : 
                    print "Error, Wrong Type U64 int expected"
                    return
                else:
                    serializeU64(frame, args[i]) 
            elif self.argTypes[i] == bool:
                if  type(args[i]) != bool : 
                    print "Error, Wrong Type bool expected"
                    return
                else:
                    serializeBool(frame, args[i]) 
            elif self.argTypes[i] == AirString:
                if type(args[i]) == AirString:
                    serialize(frame, args[i]) 
                elif type(args[i]) == str:
                    serialize(frame, AirString(args[i]))
                else:
                    return
            elif type(args[i]) == self.argTypes[i]:
#                     print "serializing {0} of type {1}".format(args[i], type(args[i]))
                    serialize(frame, args[i]) 
            else:
                print "Encapsulating failed for type of ", type(args[i])
                return
        
#         print "serialize method name {0}; afLen={1}".format(self.name, frame.getSize())
        mName = AirString(self.name)
#         print "mName.len={0} type={1}".format(mName.getLen(), type(mName))
        serialize(frame, mName)
#         frame.printFrame()
#         print "serialize type {0}; afLen={1}".format(self.type, frame.getSize())
        serializeU8(frame, self.type) # type is remote method invocation

        resp = self.sendCommand(frame)
        
        if resp == None:
            print "Remote Access Failed"
            return None
        
        retCode = unserializeU8(resp)
        if  retCode != 0:
            print "no valid response (" + str(retCode) + ")"
            return None
            
        if self.retType == None:
            return unserializeU8(resp)
        elif self.retType == uint8_t:
            return unserializeU8(resp)
        elif self.retType == uint16_t:
            return unserializeU16(resp)
        elif self.retType == uint32_t:
            return unserializeU32(resp)
        elif self.retType == uint64_t:
            return unserializeU64(resp)
        elif self.retType == bool:
            return unserializeBool(resp)
        else:
            value = self.retType();
            unserialize(resp, value);
            return value;



class RemoteModule:
    __doc__ = ''' 
    Proxy for accessing remote modules
    '''
        
    def __init__(self, remoteAccess, name, identifier=0xffff):
        self.__dict__["module"] = name
        self.__dict__["identifier"] = identifier
        self.__dict__["remoteAccess"] = remoteAccess
        
    def getId(self):
        return self.__dict__["identifier"] 
    
    def setIdentifier(self, indentifier):
        self.__dict__["identifier"] = identifier 
    
    def sendCommand(self, frame):
        # print "serialize module name {0}; afLen={1}".format(self.__dict__["module"], frame.getSize())
        serialize(frame, AirString(self.__dict__["module"]))
        return self.__dict__["remoteAccess"].sendCommand(frame,self.__dict__["identifier"])

    def declareMethodRaw(self, name, retType, isAsync, *argTypes):
        internal_name = "__" + name;
        if self.__dict__.has_key(internal_name):
            print "Error attribute with method name " + internal_name + " already exists"
            return
        self.__dict__[internal_name] = RemoteMethod(self.sendCommand, name, retType, isAsync, *argTypes)
        
    
    def declareMethod(self, name, retType, *argTypes):
        self.declareMethodRaw(name, retType, False, *argTypes)
    
    def declareMethodAsync(self, eventName, eventType, eventCallback, name, retType, *argTypes):
        ''' Combines remote method with a "taskDone" event, meaning that  
        a result is returned immediately by the target module. Iff the return
        value is RA_SUCCESS, the module will additionally issue an event as soon as
        the requested operation is completed.
        @param eventName name of the expected event
        @param type of the data contained in the event
        @param eventCallback method to be called when event is received
        @param name name of the remote method
        @param retType type of the return value of the remotely accessed method
        @param argTypes type of the arguments for the method call '''
        
        self.declareMethodRaw(name, retType, True, *argTypes)
        if not self.__dict__.has_key("__" + eventName):
	        self.declareEvent(eventName, eventType)        
	        getattr(self, eventName).register(eventCallback)
        else:
            if getattr(self, eventName).callback != eventCallback:
                print "Trying to bind different callback function to existing async method finished event"
            else:
                pass
        		#print "C" 
                #getattr(self, eventName).register(eventCallback)

    def declareVariable(self, name, remoteType):
        internal_name = "__" + name;
        if self.__dict__.has_key(internal_name):
            print "Error attribute with variable name already exists"
            return
        self.__dict__[internal_name] = RemoteVariable(self.sendCommand,name, remoteType)
    
    def declareEvent(self, name, eventType):    
        internal_name = "__" + name;
        if self.__dict__.has_key(internal_name):
            print "Error attribute with same name already exists"
            return
        self.__dict__[internal_name] = RemoteEvent(self.sendCommand,name, self.__dict__["module"], eventType,self.__dict__["identifier"]);
        
    def __getattr__(self, name):
#    	print "RemoteModule::__getattr__(" + name + ")"
        internal_name = "__" + name;

        if not self.__dict__.has_key(internal_name):
#             print "Error attribute " + str(name) + " doesn't exist"            
            return None
        if isinstance(self.__dict__[internal_name], RemoteVariable):
            #print "get attribute value"
            return self.__dict__[internal_name].get();
        else:
            return self.__dict__[internal_name]

    def __setattr__(self, name, value):
        internal_name = "__" + name;
        if not self.__dict__.has_key(internal_name):
            print "Error attribute doesn't exist"            
            return
        if isinstance(self.__dict__[internal_name], RemoteVariable):
            return self.__dict__[internal_name].set(value);
        else:
            print "Error: not supported"
            return

    def bind(name, callback):
        print "binds ", name, " to callback"



class RemoteAccess(EndpointWrap):   
    __doc__ = '''
    Remote Access Module
    '''

    def setWaitingTime(self,waitingTime):
        self.waitingTime=waitingTime

    def __init__(self, name=None):
        if name == None:
            EndpointWrap.__init__(self)
        else:
            EndpointWrap.__init__(self, name)
        self.seq = 0
        self.ongoing={}
        self.tstamps={}
        self.results={}
        self.lock=Lock()
        self.frame=None
        self.taget=None
        self.waitingTime=3.0
    
    def eventWaitMs(self, lock, event):
        start = lib.getTime();
        while True:
            self.lock.acquire()
            if event.is_set():
                self.lock.release()
                break
            elif (lib.getTime() - start) < self.waitingTime:
                self.lock.release()
                time.sleep(0.001) 
            else:
                self.lock.release()
                break
            
        
    def sendCommand(self, frame, target):

        # send data
        self.lock.acquire()
        # seq of 255 is used for identifying events
        self.seq = (self.seq+1) % 255 
                
        self.frame=frame
        self.target=target
        cseq= self.seq
        if self.ongoing.has_key(cseq):
            if not self.tstamps.has_key(cseq):
                print "Critical error, apparently used sequence number without timestamp"
                return None
            
            tsdiff = lib.getTimeMs() / 1000.0 - self.tstamps[cseq]
            if tsdiff > MAX_REMOTE_TIMEOUT_SEC:
                self.ongoing.pop(cseq)
            else:
                self.lock.release()
                print "Sequence number " + str(cseq) + " already in use (for " + str(tsdiff) + " seconds " 
                return None
        
        event= Event()
        self.ongoing[cseq] = event
        
        # remember timestamp to remove possibly leaked sequence number in future
        self.tstamps[cseq] = lib.getTimeMs() / 1000.0
        
        # within the timer.fired method, we actually send the frame
        self.setTimer(0)        
        self.lock.release()

        
        # wait for data
        #event.wait(self.waitingTime)
        
        # alternative, because event.wait uses a strange mechanism for sleeping
        self.eventWaitMs(self.lock, event)
        
        #read data
        self.lock.acquire()
        self.ongoing.pop(cseq)
        
        if not event.is_set():
            print "Timeout occured, no data received"
            if self.results.has_key(cseq):
                print "CRITICAL ERROR 1 IN REMOTE ACCESS"
            self.lock.release()
            return None
        
        if not self.results.has_key(cseq):
            print "CRITICAL ERROR 2 IN REMOTE ACCESS"  
            self.lock.release()
            return None  
            
        resp=self.results.pop(cseq)
        self.lock.release()
        return resp
    
    def initialize(self):
        pass
        
    def timeout(self):
        # print "serialize seq num {0}; afLen={1}".format(str(self.seq), self.frame.getSize())
        serializeU8(self.frame, self.seq)
        # print "sending request; afLen={0}".format(self.frame.getSize())
        self.sendRequest(self.frame, self.target)
        pass
        
    def recvIndication(self, frame2, src, dst):
        global remoteEventCallback
        
        # TODO copy of array should be done more efficient
        frame = AirframeData()
        for i in range(vectorLength(frame2)):
            vectorPush(frame, vectorGet(frame2, i));
            
        self.lock.acquire()
        cseq=unserializeU8(frame)
        
        # cseq=255 means this is an incoming event
        if cseq==255:
            self.lock.release()
            
            counter = unserializeU16(frame)
            
            mod=AirString()
            event=AirString()
            unserialize(frame, mod )
            unserialize(frame, event )
            
            id=str(src)
#            modHash = str(mod.str).rstrip('\0')
#            eventHash = str(event.str).rstrip('\0')
            modHash = str(mod.getStr())
            eventHash = str(event.getStr())
            
#            print id + "|" + modHash + "|" + eventHash
            
            if id not in remoteEventCallback:
                print "Event from " + id + " discarded; ID not found"
                return
            
#            print remoteEventCallback[id]
#            print remoteEventCallback[id].keys()
#            print modHash + "|" + str(remoteEventCallback[id].has_key(modHash)) + "|" + str(modHash in remoteEventCallback[id])
            if modHash not in remoteEventCallback[id]:
                print "Event from " + id + " discarded; Mod (" + modHash + ") not found"
                return
            
#            print "id=" + str(id) + "|modHash=" + str(modHash) + "|eventHash=" + str(eventHash) + "|mod.str=" + mod.str
            
            if eventHash not in remoteEventCallback[id][modHash]: 
                print "Event from " + id + " discarded; event (" + eventHash + ") not found"
                return
            
            remoteEventCallback[id][modHash][eventHash].call(int(id),frame)
               
            return
        elif not self.ongoing.has_key(cseq):
            self.lock.release()
            return
        self.results[cseq]=frame
        self.ongoing[cseq].set()
        self.lock.release()
        
    def recvConfirm(self, success):
        pass


