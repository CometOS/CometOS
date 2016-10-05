import firmware
import time

# Call:   [list of nodes], firmware file, slot
# example ex.otap([0],"test.hex",1)
assert len(args)==8 or len(args)==7

nodeList=args[0]
iniMsg=args[1]
interval=args[2]
timeout=args[3]
failedList=args[4]
doneFun=args[5]
async=args[6]
raModule=args[7]

originalWt = raModule.waitingTime
targets=range(len(nodeList))


print ""
print "----------------------------------------------------------------------"
print "OTAP_INIT of nodes " + str(targets) 

print "Firmware size (segments): " + str(iniMsg.segCount)

if async:
    for node in nodeList:
        if node.otap.init == None:
            print "registering async call/event pairs"
            node.otap.declareMethodAsync("oed", cometos.OtapTaskDone, doneFun, "init", uint8_t, cometos.OtapInitMessage)
else:
    for node in nodeList:
            if node.otap.init == None:
                node.otap.declareMethod("init", uint8_t, cometos.OtapInitMessage)


# initialize nodes for flashing
for target in targets:
    print "Initiation of "+hex(nodeList[target].id)+" index "+str(target) + "... ",
    raModule.setWaitingTime(timeout)
    a = cometos.palLocalTime_get()
    error=nodeList[target].otap.init(iniMsg)
    raModule.setWaitingTime(originalWt)
    if 0!=error:
        print "OTAP Initiate failed: Error "+str(error)+" Node "+hex(nodeList[target].id)
        if error != None:
            code = error
        else:
            code = 9 # PAL_FIRMWARE_ERROR
        doneFun(nodeList[target].id, cometos.OtapTaskDone(code, OTAP_ERASE))
        failedList.add(target)
    else:
        print "OK; " + str(cometos.palLocalTime_get() - a) + " milliseconds"


for target in targets:
    if not target in failedList:
        print "Setting OTAP transfer module interval "+hex(nodeList[target].id)+" index "+str(target) 
        nodeList[target].otap.si(interval)