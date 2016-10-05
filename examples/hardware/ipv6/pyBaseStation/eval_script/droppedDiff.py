''' 
Only to be used for OLD experiment logfiles, where 
basestation modules are not resetted --- calculates
difference between increasing values to get values
per experiment run. Runs need to be iterated the same way
like in the original experiment for this to work.
'''

import sys
sys.path.append('/home/anwei/Documents/projects/workspace/cometos_v6/platform/pyBaseStation')
import os
import getopt
import re
from modules.eval import *

def usage():
  print "invalid params"

def main(argv):
    inDir = "data_orig"
    stats = "lowpan_udp_statsToDiff.dat"
    outdir = "data"
    bsNodeId = 0
    print ""
    try:                                
      opts, args = getopt.getopt(argv, "d:o:m:", ["source-dir=", "outdir=", "stats="])
    except getopt.GetoptError:           
      usage()                          
      sys.exit(2)     
    for opt, arg in opts:
        if opt in ('-d', '--source-dir'):
            inDir = arg
            print "Using " + inDir + " as input directory"
            pass
        elif opt in ('-o', '--outdir'):
            outdir = arg
            print "Using " + outdir + " as output file"
            pass
        elif opt in ('-m', '--stats'):
            stats = arg
            print "Using " + stats + " as parameter stats"
    
    if not os.path.exists(inDir):
        print "Input dir not existing"
        sys.exit(1)
    if not os.path.exists(stats):
        print "Stats not existing"
        sys.exit(1)
        
    process(inDir, outdir, stats)


def process(dir, out, stats):
    # read in stats to extract
    statFh = open(stats, 'r')
    stats = []
    oldvals = {}
    s = ""
    for line in statFh:
        currStr = line.rstrip()
        stats.append(currStr)
        oldvals[currStr] = 0
        s += currStr + "\n"
    
    print s
    
    payloads = [50, 100, 150, 200]
    rates = [64, 32, 16]
    runs = range(10)
    for run in runs:
        for rate in rates:
            for payload in payloads:
                runStr = "payload="+str(payload)+"_rate="+str(rate)+"_run="+str(run)
                if not os.path.exists(str(dir) + "/" + runStr):
                    continue
                fh = open(str(dir) + "/" + runStr, 'r')
                to = open(str(out) + "/" + runStr, 'w')
                
                line = fh.readline()
                nvPairs = line.split()
                s += ""
                for pair in nvPairs:
                    e = Entry(None, pair, "=", ":")
                    if e.name in stats:
                        currval = int(e.value)
                        lastval = oldvals[e.name]  
                        diff = currval - lastval
                        if diff < 0:
                            diff += 65536
                            assert(diff > 0)
                        to.write(e.name + "=" + str(diff) + " ")
                #        print "replacing value for " + e.name + " (" + str(currval) + ") by " + str(diff) + "; old=" + str(lastval)
                        oldvals[e.name] = int(e.value)
                    else:
                        to.write(pair + " ")
                to.write("\n")
                
                for line in fh:
                    to.write(line)
                
                fh.close()    
                to.close()
                


main(sys.argv[1:])

