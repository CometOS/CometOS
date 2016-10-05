import time

# Call:   [list of nodes], firmware file, slot
# example ex.otap([0],"test.hex",1)
assert len(args)==3

nodeList=args[0]
slot=args[1]
delay=args[2]

targets=range(len(nodeList))
otap_remain = list(targets)
tmp = list(otap_remain)

tries=5

while len(otap_remain) > 0 and tries > 0:
    print "starting otap.run, nodes left: " + str(otap_remain)
    for n in otap_remain:
        print "starting otap.run(" + str(slot) +","+str(delay)+") at " + hex(nodeList[n].id) + "...",
        result = nodeList[n].otap.run(slot, delay)
        print str(result)
        if result == 0:
            tmp.remove(n)
    tries -= 1
    otap_remain = list(tmp)

if len(otap_remain) > 0:
    print "otap_run failed at " + str(otap_remain)