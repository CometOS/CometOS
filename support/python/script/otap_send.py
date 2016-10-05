import firmware
import time

# Call:   [list of nodes], firmware file, slot
# example ex.otap([0],"test.hex",1)
assert len(args)==5

nodeList=args[0]
segs=args[1]
useUnicast=args[2]
failedList=args[3]
bsOtapModule=args[4]

targets=range(len(nodeList))

def getMissing(numVecs, target):
    global nodeList
    count = 0
    missingList = []
    for i in range(numVecs):
        print "Getting Vector " + str(i) + "... ",
        missing=nodeList[target].otap.gmv(i)
        missingList.append(missing)
        if missing == None:
            print " FAIL"
            return None;
        else:
            count += missing.count(True)
            print " counted " + str(count) + " segments"
     
    return (missingList, count)
 
print ""
print "----------------------------------------------------------------------"
print "OTAP_SEND to nodes " + str(set(targets).difference(failedList)) 

for target in targets:
    if useUnicast:
        destAddr = nodeList[target].id
    else:
        destAddr = 0xFFFF
     
    if target in failedList:
        continue
     
    # get vector of missing segments, first retrieve number of segment vectors
    numVecs = nodeList[target].otap.gnmv()
    if numVecs == None:
        failedList.add(target)
        continue
     
    missResult = getMissing(numVecs, target) 
    if missResult == None:
        failedList.add(target)
        continue
    (missing, count) = missResult
    print "Arrived on node " + hex(nodeList[target].id) + ": " + str(count) + " segments"
    # now check all vectors and compare with segment count and repeat sending
    # segments which are missing
     
    # note that missing.count is currently only working for True
    while count!=len(segs):
        for i in xrange(len(segs)):
            if not missing[i/OTAP_BITVECTOR_SIZE].get(i%OTAP_BITVECTOR_SIZE):
                time.sleep(0.1)
                     
                bsOtapModule.sendSegmentBlocking(destAddr,segs[i],i)
                print "Flash seg "+str(i)+" nodes "+hex(nodeList[target].id)+" progr "+ str(100*(i+1)/(len(segs)))+" %"
        time.sleep(0.2)
        (missing, count) = getMissing(numVecs, target)
        if count == None:
            failedList.add(target)
            break


