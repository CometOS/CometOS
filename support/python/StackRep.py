from types import *


class Module:
    '''To represent a module'''
    
    moduleMap = {}
    
    def __init__(self, type, varName, package, shortName=None, targets=['hw','py','sim'], templateArgs=None, encl=None, ctorParams=[]):
        self.type=type
        self.name=varName
        self.sname = shortName
            
        self.tArgs = templateArgs
            
        self.package = package
        self.targets = targets
        self.encl = encl
        self.ctorParams = ctorParams
        Module.moduleMap[self.name] = self

    def checkTarget(self, target):
        return target in self.targets or self.targets=='all'

class Varname:
    pass

class Gate:
    def __init__(self, mod, gate, gateIdx=None):
        self.m = mod
        self.g = gate
        self.idx = gateIdx
        

class Link:
    ''' Represents a connection between gates of modules'''
    list = []
    
    def __init__(self, srcGate, dstGate, encl=None):
        self.g1 = srcGate
        self.g2 = dstGate
        self.encl = encl
        Link.list.append(self)
            
    
    
class StackWriter(object):
    def __init__(self, modules, links):
        self.modules = modules
        self.links = links
    
    def write(self, filename):
#         dir = os.path.dirname(filename)
#         if not os.path.exists(dir):
#             print("Making dir " + dir + " ")
#             os.makedirs(dir)
#         file = open(filename, 'w')
        s = ""
        s += self.getImportString()
        s += "\n"
        s += self.getModuleString()
        s += "\n"
        s += self.getLinkString()
        
        print s
        pass
    
    def checkLink(self, l):
        pass
    
    def raiseTargetInconsistency(self, moduleName):
        raise(Exception("Module " + moduleName + " not existing in this build"))
        
    def getImportString(self):
        raise(Exception("Has to be overridden!"))
    
    def getModuleString(self):
        raise(Exception("Has to be overridden!"))
    
    def getLinkString(self):
        raise(Exception("Has to be overridden!"))



class PlatformStackWriter(StackWriter):
    conStr = "connectTo"
    gateArrayCmd="get"
    
    def __init__(self, modules, links):
        super(PlatformStackWriter, self).__init__(modules, links)
        
        
    
    def getImportString(self):
        s = ""
        
        
    def getModuleString(self):
        s = "" 
        for m in self.modules:
            pass
        return s
    
    def getLinkString(self):
        s = ""
        for l in self.links:
            if self.checkLink(l):
                s += self.printLink(l) 
                s += "\n"
        return s
    
    def printLink(self, link):
#         print("printing link")
        return self.printGate(link.g1) + "." + PlatformStackWriter.conStr + "(" + self.printGate(link.g2) + ")"
    
    def printGate(self, gate):
#         print("printing gate")
        s = gate.m + "." + gate.g;
#         print(gate.m + "." + gate.g)
        if gate.idx != None:
            s += "." + PlatformStackWriter.gateArrayCmd + "(" + str(gate.idx) + ")"
        return s
   
   

class PyStackWriter(PlatformStackWriter):
    pass


class CppStackWriter(PlatformStackWriter):
    def __init__(self, modules, links):
        super(CppStackWriter, self).__init__(modules, links)
        self.tName="hw"
    
    def checkLink(self, l):
#         print(str(self.modules))
        p1 = l.g1.m in self.modules and Module.moduleMap[l.g1.m].checkTarget(self.tName)
        p2 = l.g2.m in self.modules and Module.moduleMap[l.g2.m].checkTarget(self.tName)
        return p1 and p2

    def checkModule(self, m):
        return self.tName in m.targets
    
    def getImportString(self):
        s= ""
        for m in self.modules.values():
            if m.checkTarget(self.tName):
                s+= '#include "' + m.type + '.h"\n'
        return s
    
    def getModuleString(self):
        s = ""
        for m in self.modules.values():
            if m.checkTarget(self.tName):
                s += m.package.split(".")[0] + "::" + m.type + " " + m.name + "("
                if m.sname != None:
                    s += m.sname
                    s += ", "
                for p in m.ctorParams:
                    if type(p) is IntType or type(p) is FloatType or type(p) is LongType:
                        s += str(p)
                    elif type(p) is StringType:
                        s += '"' + p + '"'
                    else: 
                        s += str(p)
                    s += ", "
                
                s = s[:-2]
                s += ");"
                s += "\n"
        return s
    
            

class NedStackWriter(StackWriter):
    pass



### Test script to get all use cases

# enclosures
serialPrintf="SERIAL_PRINTF"
notSerialPrintf=["#ifndef " + serialPrintf,"#endif"]
doSerialPrintf=["#ifdef " + serialPrintf,"#endif"]


Module(type="SerialComm", package="cometos.modules", varName="comm", encl=notSerialPrintf, targets=["hw","py"])
Module(type="NetworkInterfaceSwitch", package="cometos.mac", varName="nis", encl=notSerialPrintf)
Module(type="CsmaMac", package="cometos.mac", varName="mac")
Module(type="Dispatcher", package="cometos.comm", varName="macDisp")
Module(type="SimpleRouting", package="cometos.routing", varName="sr")
Module(type="Dispatcher", package="cometos.comm", varName="nwkDisp", templateArgs=[4])

Link(Gate("sr", "gateIndOut"), Gate("nwkDisp", "gateIndIn"))
Link(Gate("nwkDisp", "gateReqOut"), Gate("sr", "gateReqIn"))

Link(Gate("macDisp", "gateIndOut", 0), Gate("sr", "gateIndIn"))
Link(Gate("sr", "gateReqOut"), Gate("macDisp", "gateReqIn", 0))

Link(Gate("macDisp", "gateReqOut"), Gate("mac","gateReqIn"), encl=doSerialPrintf)
Link(Gate("mac", "gateIndOut"), Gate("macDisp", "gateIndIn"), encl=doSerialPrintf)

Link(Gate("nis","gateIndOut"), Gate("macDisp", "gateIndIn"), encl=notSerialPrintf)
Link(Gate("macDisp", "gateReqOut"), Gate("nis", "gateReqIn"), encl=notSerialPrintf)

Link(Gate("nis", "cniReqOut"), Gate("mac","gateReqIn"), encl=notSerialPrintf)
Link(Gate("mac", "gateIndOut"), Gate("nis","cniIndIn"), encl=notSerialPrintf)

Link(Gate("nis", "nniReqOut"), Gate("comm","gateReqIn"), encl=notSerialPrintf)
Link(Gate("mac", "gateIndOut"), Gate("nis","nniIndIn"), encl=notSerialPrintf)




csw = CppStackWriter(Module.moduleMap, Link.list)
csw.write("")