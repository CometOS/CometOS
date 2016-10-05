import time

assert(len(args)>=4)

## name of the experiment. appended to the current month
name = args[0]

## String containing a whitespace separated list of modes to execute 
## used to defer the delay mode setting and the name of the results dir
mode = args[1]

## contains a list of nodes which should be senders
nodeList = args[2] 

## boolean parameter which decides if nodes send at the same time or one after another
sendSimultaneously = args[3]


## check if we want to output the experiment state via an output pin of the 
## raspberry pi
sendSignal = None
if len(args) > 4:
    sendSignal = args[4]

expName=time.strftime("%Y-%m") + "_" + name


## default setting is without any retry control
macRtxControl = LOWPAN_MACCONTROL_DEFAULT

modes = mode.split()

baseLogDir = "/home/anwei/Documents/projects/lowpan_fwd/log/experiments/"
expLogDir = baseLogDir + expName + "/"

for m in modes:
    if m == "Direct" or m == "Assembly":
        delayMode = DM_NONE
    elif m == "Direct-ARR":
        delayMode = DM_ADAPTIVE
    else:
        print "Invalid mode " + m + "; valid are Direct, Direct-ARR, Assembly"
    
    if sendSimultaneously:
        modeLogDir = expLogDir + "/" + m
        ### "normal" execution, all nodes are senders
        ex.exp_udp(IM_UNIFORM, cometos.LowpanConfig(macRtxControl, delayMode), nodeList, modeLogDir, sendSignal)
    else:
        for n in nodeList:
            modeLogDir =  expLogDir + "/node_" + str(n.id) + "/" + m
            print "Executing with node " + str(n.id) + " as sender"
            ex.exp_udp(IM_FIXED, cometos.LowpanConfig(macRtxControl, DM_NONE), [n], modeLogDir, sendSignal)
