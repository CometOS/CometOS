import time
from datetime import datetime
from stack.helpers import *
from Experiment import *
import modules.cfg
import modules.RaspiPinOut
import os
import modules.expcfg as expcfg 

allNodes = nodes
targetNodes = nodes

tsMasterId=0x100
if len(args) > 0:
    targetNodes=args[0]
if len(args) > 1:
    cfgFile = args[1]
if len(args) > 2:
    logDirName=args[2]
if len(args) > 3:
    tsMasterId = int(args[3])



tsMaster = None
for n in allNodes:
    if int(n.id) == tsMasterId:
        tsMaster = n
print "Using {0:x} as time sync master".format(tsMaster.id)
	
print "Results will be logged to " + logDirName
subdirs = ["/data/", "/meta/", "/detail/"]
 
for d in subdirs:
	if not os.path.exists(logDirName +  d):
		os.makedirs(logDirName + d)


(pars, vals, calcs) = expcfg.parse("test.cfg")
cfgs=expcfg.createObjects(pars, vals, calcs)
expDuration = 0.0

tgobj = next(obj for obj in cfgs[0] if type(obj) == cometos.TgConfig)
tgIdx = 0
for i in range(len(cfgs[0])):
    if type(cfgs[0][i]) == cometos.TgConfig:
         print "Found obj " + str(type(cfgs[0][i])) + " at tgIdx " + str(i)
         tgIdx = i
    
     
for runcfg in cfgs:
    tgcfg = runcfg[tgIdx]
    thisrun= (1.0 * tgcfg.maxRuns) / (tgcfg.rate/256.0) + (tgcfg.offset + tgcfg.finishDelay) / 1000
    expDuration += thisrun
    print "add {0}s ({5}h) to experiment duration; numDgs={1}|rate={2}|offset={3}|finiDelay={4}".format(thisrun, tgcfg.maxRuns, tgcfg.rate/256.0, tgcfg.offset/1000, tgcfg.finishDelay/1000, thisrun/3600)
    
print "Expected experiment duration: {0}h".format(expDuration/3600)

runs=5
runNr = 0

for i in range(len(cfgs)):
    for run in range(runs):
        runStr = "run_{0:03}".format(runNr)
        runNr += 1
        logFilename = "{0}/{1}/{2}".format(logDirName, subdirs[0], runStr)
        logMetaFilename = "{0}/{1}/{2}".format(logDirName, subdirs[1], runStr)
        logDetailFilename = "{0}/{1}/{2}".format(logDirName, subdirs[2], runStr)
        with open(logFilename, 'w') as log, open(logMetaFilename, 'w') as logMeta, open(logDetailFilename, 'w') as logDetail:
            runcfgs = []
            postcfgs = []
            paramcfgs = []
            
            ## configure time sync
            tssSlaves = list(allNodes)
            tssSlaves.remove(tsMaster)
            tssSlaves.append(bs)
            tssCfg = Tss(NodeCfgMethod(RoleCfg(False), "tss", "start"), NodeCfgMethod(None, "tss", "stop"), tssSlaves, tsMaster)
            tssGetNum = NwkCfg(NodeCfgMethod(None, "tss", "gnt", logMeta), allNodes)
    
            ## some utility methods to be called
            heapUtil = NwkCfg(NodeCfgMethod(None, "sys", "util"), allNodes)
            resetStats = NwkCfg(NodeCfgMethod(None, LOWPAN_MODULE_NAME, "rs"), allNodes)        
    
            precfgs = [tssGetNum, tssCfg, heapUtil, resetStats]
            #precfgs = [tssGetNum, heapUtil, resetStats]
            postcfgs = [tssGetNum, heapUtil]
            
            getLowpanStats = NwkCfg(NodeCfgMethod(None, LOWPAN_MODULE_NAME, "gs"), allNodes)
            getLoofStats = NwkCfg(NodeCfgMethod(None, LowpanOrderedForwarding.MODULE_NAME, "gs"), allNodes)
            collectcfgs = [getLowpanStats]
    
            lcaType = 0
            ## we just use TgConfig as run config, the others are added to paramcfgs, which are directly logged by the experiment
            for cfg in cfgs[i]:
                if type(cfg) == cometos.TgConfig:
                    for node in targetNodes:
                        cond = NodeEvent(node.tg, "tf")
                        method = NodeCfgMethod(cfg, "tg", "run")
                        runcfgs.append(RunCfg(method, cond, node))
                elif type(cfg) == cometos.MacConfig:
                    paramcfgs.append(NwkCfg(NodeCfgMethod(cfg, MAC_MODULE_NAME, "ssc"), allNodes))
                    precfgs.append(NwkCfg(NodeCfgMethod(cfg, MAC_MODULE_NAME, "ssc"), [gw]))
                elif type(cfg) == cometos.LowpanConfig:
                    #### NOTE, if the basestation starts sending, too, we need to configure it here, too
                    paramcfgs.append(NwkCfg(NodeCfgMethod(cfg, LOWPAN_MODULE_NAME, "sc"),allNodes))
                    lcaType = cfg.congestionControlType                    
                    #TODO configure basestation
                elif type(cfg) == cometos.IpConfig:
                    paramcfgs.append(NwkCfg(NodeCfgMethod(cfg, IPFWD_MODULE_NAME, "sc"), allNodes))
                    pass
                elif type(cfg) == cometos.TxPower:
                    paramcfgs.append(NwkCfg(NodeCfgMethod(cfg, TX_POWER_LAYER_MODULE_NAME, "spl"), allNodes + [bs]))
                else:
                    assert(type(cfg)== LowpanOrderedForwardingCfg)
            
            if lcaType == LowpanConfigConstants.CCT_DG_ORDERED:
                collectcfgs.append(getLoofStats)
                
            ## second pass to set LOOF configuration, if necessary
            for cfg in cfgs[i]:
                if type(cfg) == cometos.LowpanOrderedForwardingCfg and lcaType == LowpanConfigConstants.CCT_DG_ORDERED:
                    paramcfgs.append(NwkCfg(NodeCfgMethod(cfg, LowpanOrderedForwarding.MODULE_NAME, "sc"), allNodes))
                    
            print "======================================================================================"
            print "======= Running experiment    ========================================================"
            print "======================================================================================"
            print "Pre:\n     {0}".format(str(precfgs))
            print "----------------------------------------------------------------"
            print "Post:\n    {0}".format(str(postcfgs))
            print "----------------------------------------------------------------"
            print "Collect:\n {0}".format(str(collectcfgs))
            print "----------------------------------------------------------------"
            print "Params:\n  {0}".format(str(paramcfgs))
            print "----------------------------------------------------------------"
            print "Run:\n     {0}".format(str(runcfgs))
            tgcfg = cfgs[i][tgIdx]
            runTimeout = 256.0/tgcfg.rate * tgcfg.maxRuns * 1.5 * 1000 + tgcfg.finishDelay + tgcfg.offset + 10000
            
            e = UdpExperiment(allNodes, precfgs, paramcfgs, runcfgs, tg, postcfgs, collectcfgs, runTimeout, log, run)
            e.pre()
            e.run()
            e.post(logDetail)
            e.logData(bs)
            e.cleanup()
            log.flush()
                    

