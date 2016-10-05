import firmware
import time
import argparse

''' Call:  
          useAsync, 
          list of nodes to flash, 
          firmware file, 
          slot, 
          sendIntervalMs, 
          remoteAccessModule, 
          baseStationOtapModule,
          [useUnicast=false], 
          [remote timeout for initialization and verification in seconds]
          [string containing operations to perform, i.e. "isv" means (i)nitialize, (s)end, (v)erify]
          '''

useArgParse = False

exited = False

if useArgParse:
    class ArgumentParser(argparse.ArgumentParser):    
        def error(self, message):
            exc = sys.exc_info()[1]
            if exc:
                raise exc
            super(ArgumentParser, self).error(message)
    
    
    # example ex.otap(False, [0],"test.hex", 1, 75, false, 30)
    toPerSeg = 0.075
    do="isv"
    
    parser = ArgumentParser(description='Initialize/Execute/Verify transfer of a firmware image.')
    parser.add_argument("filename", type=str, help="Path to the firmware image (Intel hex format)")
    parser.add_argument("slot", type=int, help="Slot where the image should be stored")
    parser.add_argument("nodes", type=list, help="List of nodes to transfer firmware to")
    parser.add_argument("--interval", "-i", type=int, default=65, help="Interval between sending of packets in milli seconds")
    parser.add_argument("--unicast", "-u", type=bool, default=False, help="Use unicast transport, e.g., if only a single node is target")
    parser.add_argument("--timeout", "-t", type=float, default=10.0, help="Timeout for verification/initialization in seconds")
    parser.add_argument("--doOperations", "-d", type=str, default="isv", help="May contain up to three letters: i:initialization s:send v:verify")
    parser.add_argument("--remoteModule", "-r", type=object, default=remote, help="Specifies the remote module to be used to temporarily change timeout values depending on firmware size")
    parser.add_argument("--otapModule", "-o", type=object, default=otap, help="Specify the basestation's otap module used to transfer the data")
    parser.add_argument("--asynchronous","-a", type=bool, default=True, help="If set, uses asynchronous communication when initializing and verifying")
    
    
    
    try:
        args = parser.parse_args(args)
        print args
        async=args.asynchronous
        nodeList=args.nodes
        filename=args.filename
        slot=args.slot
        interval=args.interval
        raModule=args.remoteModule
        bsOtapModule=args.otapModule
        timeout = raModule.waitingTime
        useUnicast = args.unicast
        timeout = args.timeout
        do = args.doOperations
    except SystemExit:
        exited = True
else:
    do = "isv"
    
    async=args[0]
    nodeList=args[1]
    filename=args[2]
    slot=args[3]
    interval=args[4]
    raModule=args[5]
    bsOtapModule=args[6]
    timeout = raModule.waitingTime
    if len(args) > 7:
        useUnicast = args[7]
        if len(args) > 8:
            timeout = args[8]
        if len(args) > 9:
            assert(len(args)==10)
            do = args[9]
            
    else:
        assert len(args)==7
        useUnicast = False

if not exited:
    originalWt = raModule.waitingTime
    ## we used to pass a list of indices, instead of a list of nodes, to not have
    ## to change all scripts, we just regenerate such a list from the provided
    ## nodeList
    targets = range(len(nodeList))
    
    
    # load firmware from file
    (startaddr,segs,crc)=firmware.loadFirmwareFromHex(filename)
    print str(type(slot)) + " " + str(type(len(segs))) + " " + str(type(crc))
    iniMsg = OtapInitMessage(slot,len(segs),crc)
    
    failedList = set([])
    
    if timeout==None:
        timeout = len(segs) * toPerSeg + 1
    
    print "Starting OTAP; timeout for initialization/verification at nodes="+str(timeout)
    print "Image information: numSegments=" + str(len(segs)) + " startAddr=" + hex(startaddr) + " crc=" + str(crc)
    print "Setting frame interval at basestation to {0}".format(interval) 
    bsOtapModule.setInterval(interval)
    
    
    def findIndexForId(idxList, id):
        global nodeList
        for idx in idxList:
            if int(id) == int(nodeList[idx].id):
                return idx
        return None    
    
     
    def operationDone(node, taskDoneEvent):
        global targets, failedList, doneEvent, doneSet, currOp, findIndexForId
        print "Received finish event (Op=" + str(taskDoneEvent.opId) + "|Status=" + str(taskDoneEvent.status) +") from node " + hex(int(node)) 
        #+ "|Size=" + str(taskDoneEvent.size) + "|FwVersion=" + str(taskDoneEvent.version) + 
        assert(currOp == taskDoneEvent.opId)
        nodeIdx = findIndexForId(targets, node)
        assert(nodeIdx != None)
        doneSet.add(nodeIdx)
        if (taskDoneEvent.status != 0):  # 0 = PAL_FIRMWARE_SUCCESS
            print "Node "+ hex(int(node)) + " failed with status: " + str(taskDoneEvent.status)
            failedList.add(nodeIdx)
        if (doneSet.union(failedList).issuperset(targets)):
            print "All nodes finished: success at: " + str(doneSet.difference(failedList)) + " failed at: " + str(failedList)
            doneEvent.set()
    
    
    def printErrorIfEventNotSet(event, error):
        if not event.isSet():
                print error
    
    if not async:   
        # initialize nodes for flashing (erase slot, set broadcasting interval etc.)
        ex.otap_init(nodeList, iniMsg, interval, timeout, failedList, async, raModule)
        
        ex.otap_send(nodeList, segs, useUnicast, failedList, bsOtapModule)
        
        ex.otap_veri(nodeList, (startaddr, crc), timeout, failedList, async, raModule)
        
    else:
        doneEvent = Event()
        if "i" in do:
            doneEvent.clear()
            doneSet = set([])
            currOp = OTAP_ERASE
            ex.otap_init(nodeList, iniMsg, interval, originalWt, failedList, operationDone, async, raModule)
            doneEvent.wait(timeout)
            printErrorIfEventNotSet(doneEvent, 'Timeout occurred while waiting for "initialization finsihed" events')
        
        if "s" in do:
            ex.otap_send(nodeList, segs, useUnicast, failedList, bsOtapModule)
        
        if "v" in do:
            doneEvent.clear()
            doneSet = set([])
            currOp = OTAP_VERIFY
            if not failedList.issuperset(targets):
                ex.otap_veri(nodeList, slot, timeout, failedList, operationDone, async, raModule)
                doneEvent.wait(timeout)
                printErrorIfEventNotSet(doneEvent, 'Timeout occurred while waiting for "verification finsihed" events')
            
            if len(failedList) > 0:
                print "OTAP failed at "
                for i in failedList:
                   print hex(nodeList[i].id)
            else:
                print "OTAP verification done"
