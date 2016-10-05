import firmware
import time


assert len(args)==7 or len(args)==6

nodeList=args[0]
params=args[1]
eepromTo=args[2]
failedList=args[3]
doneFun=args[4]
async=args[5]
raModule=args[6]

originalWt = raModule.waitingTime
targets=range(len(nodeList))

print ""
print "----------------------------------------------------------------------"
print "OTAP_VERIFY of nodes " + str(targets)

## asynchronous operation of otap
if async:
    slot = params
    for node in nodeList:
        if node.otap.veri == None:
            print "registering async call/event pairs"
            node.otap.declareMethodAsync("oed", cometos.OtapTaskDone, doneFun, "veri", uint8_t, uint8_t)
else:
    (startAddr, crc) = params
    # declare synchronous methods
    for node in nodeList:
        if node.otap.veri == None:
            node.otap.declareMethod("veri", uint8_t, uint16_t, uint32_t)
            
            
# verify firmware
for target in targets:
    raModule.setWaitingTime(eepromTo)
    error = None
    if not target in failedList: 
        print "Verify of "+hex(nodeList[target].id)+" index "+str(target)+"... ",
        if async:
            error=nodeList[target].otap.veri(slot)
        else:
            error=nodeList[target].otap.veri(crc,startaddr)
        print error
    else:
        continue
    raModule.setWaitingTime(originalWt)
    if 0!=error:
        print "OTAP Verify failed: Error "+str(error)+" Node "+hex(nodeList[target].id)
        if error != None:
            code = error
        else:
            code = 9 # PAL_FIRMWARE_ERROR
        failedList.add(target)
        if error != None:
            code = error
        else:
            code = 9 # PAL_FIRMWARE_ERROR
        doneFun(nodeList[target].id, OtapTaskDone(code, OTAP_VERIFY))
#        raise Exception("OTAP Verify failed: Error "+str(error)+" Node "+hex(nodeList[target].id))
     

         
raModule.setWaitingTime(originalWt)
