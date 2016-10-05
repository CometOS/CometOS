import sys
import os
import getopt
import random

import xml.etree.ElementTree as ET
from xml.dom.minidom import parseString


def usage(reason):
    print "Wrong usage: " + reason
    print "usage: " + sys.argv[0] + " -f <input link file> -o <outfile> -n <bsAttachedNode> -b <bsId>"

def main(argv):
    bsNodeId = None
    bsAttachedNode = None
    infile = ""
    outfile = ""
    print ""
    try:                                
      opts, args = getopt.getopt(argv, "f:o:n:b:h", ["file=", "outfile=", "bsAttachedNode=", "bsId"])
    except getopt.GetoptError:           
      usage("getopt error")                          
      sys.exit(2)     
#    if len(args) != 3:
#        usage("not all mandatory args given")
#        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-f', '--file'):
            infile = arg
            print "Using " + infile + " as input file"
            pass
        elif opt in ('-o', '--outfile'):
            outfile = arg
            print "Using " + outfile + " as output file"
            pass
        elif opt in ('-n', '--bsAttachedNode'):
            bsAttachedNode = int(arg, 16)
            print "Using " + hex(bsAttachedNode) + " as bs-attached node"
            pass
        elif opt in ('-b', '--bsId'):
            bsNodeId = int(arg, 16)
            print "Using " + hex(bsNodeId) +  " as basestation address"
            pass
        elif opt in ('-h'):
            usage()
            sys.exit(0)
    
    if not os.path.exists(infile):
        print "Input file not existing"
        sys.exit(1)
    
    process(infile, outfile, bsNodeId, bsAttachedNode)

    
def process(infile, outfile, bsId, bsAttachedNode):
    inFh = open(infile,'r')
    links = readInput(inFh)
    tree = toXml(links, bsId, bsAttachedNode)
    outFh = open(outfile, 'w')
    outFh.write(prettyPrint(tree))
    
    
def prettyPrint(element):
    txt = ET.tostring(element)
    return parseString(txt).toprettyxml()


def readInput(file):
        # ignore header
        file.readline()
        links = {}
        for line in file:
            vals = line.split()
            dst = vals[0]
            src = vals[1]
            rssiAvg = float(vals[2])
            rssiVar = float(vals[3])
            numRcvd = int(vals[4])
            etx = float(vals[5])
            if src not in links:
                links[src] = {}
            
            n = int(numRcvd * etx)
            #print str(numRcvd) + "|" + str(etx)
            numLost = n - numRcvd
            assert(numLost >= 0)
            
            if rssiAvg == -90:
                r = random.randint(0, 10)
                rssiAvg -= r
                if rssiVar == 0.0:
                    rssiVar += random.randint(0, r/2)
            links[src][dst] = (rssiAvg, rssiVar)
            
        return links
        


def toXml(links, bsNodeId, bsAttachedNode):
    nodeName = 'node'
    linkName = 'link'
    idName = 'id'
    destName = 'dest'
    rssiName = 'rssiMean'
    varName = 'rssiVar'
    
    ## TODO rather awkward use of split to remove 0x part from hex string
    
    root = ET.Element("root")
    
    if bsNodeId != None and bsAttachedNode != None:
        bsStr = hex(bsNodeId)
        bsAStr = hex(bsAttachedNode)
    
        # create BS node and outgoing link
        bsNode = ET.SubElement(root, nodeName, {idName:bsStr})
        ET.SubElement(bsNode, linkName, {destName:bsAStr, rssiName:"50", varName:"0", "per":"0"})
        
    for link in links:
        nodeStr = hex(int(link))
        currNode = ET.SubElement(root, nodeName, {idName:nodeStr})
        
        # if this is the bs-attached node, add link towards it
        if (bsAttachedNode != None and int(link) == int(bsAttachedNode)):
            ET.SubElement(currNode, linkName, {destName:bsStr, rssiName:"50", varName:"0", "per":"0"})
            
        for dest in links[link]:
            rssiStr = str(-links[link][dest][0])
            varStr = str(links[link][dest][1])
            destStr = hex(int(dest))
            ET.SubElement(currNode, linkName, {destName:destStr, rssiName:rssiStr, varName:varStr, "per":"0"})
    
    ET.ElementTree(root)
    return root


random.seed()
main(sys.argv[1:])
