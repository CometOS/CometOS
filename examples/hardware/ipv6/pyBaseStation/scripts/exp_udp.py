import time
from datetime import datetime
from stack.helpers import *
from Experiment import *
import modules.cfg
import modules.RaspiPinOut
import os

bsNode = None
for n in nodes:
    if int(n.id) == (BS_ATTACHED):
        bsNode = n
		

assert bsNode != None		

def setLowpanCfg(cfg):
	for i in range(3):
		for n in nodes:
			print hex(n.id) + "|isNormalNode=" + str(int(n.id) != BS_ATTACHED)
			if int(n.id) != BS_ATTACHED:
				print "Setting config; result=" + lib.getAttrString(n.low.setCfg(cfg))
			else:
				print "Setting config; result=" + lib.getAttrString(n.low.setCfg(cometos.LowpanConfig(cfg.macControlMode, DM_NONE)))	


lowpanCfg=cometos.LowpanConfig(LOWPAN_MACCONTROL_DEFAULT, DM_NONE)
mode = IM_FIXED
targetNodes = nodes
	
if len(args) > 0:
	mode=args[0]
if len(args) > 1:
    lowpanCfg=args[1]
if len(args) > 2:
    targetNodes=args[2]
if len(args) > 3:
    logDirName=args[3]
if len(args) > 4:
    sendSignal = args[4]
	
print targetNodes
for n in targetNodes:
	print n.id
	
# setup configuration structs, manually for now



mRTT = False # False
mRTTDest = 0
mTP = True # True 
tpDest = 0

# baseOffset = 20000
# payloads = [50, 200, 800, 1200]
# rates = [8, 12, 16, 24, 48, 96, 192, 384]   # packets every 256 sec
# byterates = [9600, 19200]
# totalDataPerNode = 120000
# finiDelay = 60000
# runs = range(5)


## test config
baseOffset = 20000
payloads = [1200]
rates = [16]   # packets every 256 sec
byterates = [19200]
totalDataPerNode = 12000
finiDelay = 20000
runs = range(1)

print "Results will be logged to " + logDirName

subdirs = ["/data/", "/meta/", "/detail/"]

for d in subdirs:
	if not os.path.exists(logDirName +  d):
		os.makedirs(logDirName + d)

expDuration = 0.0
for run in runs:
	for rate in rates:
		for payload in payloads:
			resRate = rate * payload
			if resRate not in byterates:
				continue
			expDuration += totalDataPerNode / (resRate / 256)

print "Estimated minimum experiment duration: " + str(expDuration) + "s (" + str(expDuration/3600.0) + "h)" 
for run in runs:
    for rate in rates:
        for payload in payloads:
            resRate = rate * payload
            if resRate not in byterates:
                continue
            
            setLowpanCfg(lowpanCfg)
            numDatagrams = totalDataPerNode / payload;
            runStr = "payload="+str(payload)+"_rate="+str(rate)+"_run="+str(run)
            log = open(str(logDirName)+subdirs[0] + runStr, 'w')
            logMeta = open(str(logDirName)+subdirs[1] + runStr, 'w')
            logDetail = open(str(logDirName)+subdirs[2] + runStr, 'w')
			
			# we use the bs-attached node as timesync master to prevent 
			# regular resets at that node, causing the trickle timer to reset,
			# because we suspect the transceiver crystals to be much more 
			# accurate than the pc/laptop timers
            tssSlaves = list(nodes)
            tssSlaves.remove(bsNode)
            tssSlaves.append(bs)
            tssCfg = Tss(NodeCfgMethod(RoleCfg(False), "tss", "start"), NodeCfgMethod(None, "tss", "stop"), tssSlaves, bsNode)
			
            cfgs = []
			
            tssGetNum = NwkCfg(NodeCfgMethod(None, "tss", "gnt", logMeta), nodes)
            heapUtil = NwkCfg(NodeCfgMethod(None, "sys", "util"), nodes)
            resetStats = NwkCfg(NodeCfgMethod(None, LOWPAN_MODULE_NAME, "rs"), nodes)
			
			# change targetNodes here if just some nodes should be sending data
            for node in targetNodes:
                #print str(node) + "  " + str(node.id)
                cond = NodeEvent(node.tg, "tf")
                method = NodeCfgMethod(TgConfig(baseOffset, payload, mRTT, mRTTDest, mTP, tpDest, rate, mode, numDatagrams, finiDelay), "tg", "run")
                cfgs.append(RunCfg(method, cond, node))
				
			
			#preCfgs = [tssGetNum, heapUtil]
            preCfgs = [tssGetNum, tssCfg, heapUtil, resetStats]
            #preCfgs = [resetStats]
            postCfgs = [tssGetNum, heapUtil]
            #postCfgs = []
			
            get6LoStats = NwkCfg(NodeCfgMethod(None, LOWPAN_MODULE_NAME, "gs"), nodes)
# 			getMacStats = NwkCfg(NodeCfgMethod(None, MAC_MODULE_NAME, "gs"), nodes)
			
            collectCfgs = []
            collectCfgs.append(get6LoStats)
# 			collectCfgs.append(getMacStats)
			
            e = UdpExperiment(nodes, preCfgs, cfgs, tg, postCfgs, collectCfgs, (256.0/rate * numDatagrams * 1.5 * 1000) + finiDelay + baseOffset + 10000, log)
            print "=============================================================="
            print "== run= " + str(run) + " rate=" + str(rate) + " payload=" + str(payload) +" =============="
            print "==============================================================" 
            
            ## create RaspiPinOut object which will switch 
            if sendSignal != None:
                rpo = RaspiPinOut(baseOffset / 1000 / 2, expDuration / 2, sendSignal)
                e.subscribeToEventSignal(rpo.startEvent)
                
            e.pre()
            e.run()
            e.post(logDetail)
            e.logData(bs)
            e.cleanup()
            log.flush()
            if sendSignal != None:
                e.unsubscribeFromEventSignal(rpo.startEvent)
