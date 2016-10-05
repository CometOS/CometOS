import sys
import os
import getopt
import random

import xml.etree.ElementTree as ET
from xml.dom.minidom import parseString


def usage(reason):
    print "Wrong usage: " + reason
    print "usage: " + sys.argv[0] + " -f <input link file> -o <outfileHw> -s <outfileSim> -b <bsNode>"

def main(argv):
    bsNodeId = 0
    infile = ""
    outfileHw = ""
    outfileSim = ""
    ipv6Prefix = "64:29:30:31:0:0:0:"
    print ""
    try:                                
        opts, args = getopt.getopt(argv, "f:o:s:b:h", ["file=", "outfileHw=", "outfileSim", "bsNodeId="])
    except getopt.GetoptError as err:
        print str(err)           
        usage("getopt error")                          
        sys.exit(-1)     
#    if len(args) != 3:
#        usage("not all mandatory args given")
#        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-f', '--file'):
            infile = arg
            print "Using " + infile + " as input file"
            pass
        elif opt in ('-o', '--outfileHw'):
            outfileHw = arg
            print "Using " + outfileHw + " as output file for hardware routing table"
            pass
        elif opt in ('-s', '--outfileSim'):
            outfileSim = arg
            print "Using " + outfileSim + " as output file for simulator routing table"
        elif opt in ('-b', '--bsNodeId'):
            bsNodeId = int(arg, 16)
            print "Using " + hex(bsNodeId) + " as basestation address"
            pass
        elif opt in ('-h'):
            usage()
            sys.exit(0)
    
    if not os.path.exists(infile):
        print "Input file not existing"
        sys.exit(1)
        
    process(infile, outfileHw, outfileSim, bsNodeId, ipv6Prefix)

    
def process(infile, outfile, outfileSim, bsId, ipv6Prefix):
    inFh = open(infile,'r')
    routes = readInput(inFh, bsId)
    (treeHw, treeSim) = toXml(routes, bsId, ipv6Prefix)
    outFh = open(outfile, 'w')
    outFh.write(prettyPrint(treeHw))
    outFh.close()
    outFh = open(outfileSim, 'w')
    outFh.write(prettyPrint(treeSim))
    
    
def prettyPrint(element):
    txt = ET.tostring(element)
    return parseString(txt).toprettyxml()

def readInput(file, bsNodeId):
    # read number of nodes
    numNodes = int(file.readline())
    
    nextHops = {}
    currSrc = 0xFFFF
    pred = {}
    for line in file:
        vals = line.split()
        if len(vals) == 1:
            # clear predecessor list
            currSrc = int(vals[0])
            pred[currSrc] = {}
        else:
            assert len(vals) == 3
            pred[currSrc][int(vals[0])] = int(vals[1])
    
    routes = {}
    for src in pred:
        if src == bsNodeId:
            getAllDests = True
        else:
            getAllDests = False
        
        for dst in pred[src]:
            if getAllDests or dst==bsNodeId:
#                print "processing path from " + str(src) + " to " + str(dst)
                nextHop = dst
                while True:
                    currNode = pred[src][nextHop]
                    if not currNode in routes:
                        routes[currNode] = {}
#                    print "added route at " + str(currNode) + " for " + str(dst) + "; nextHop: " + str(nextHop)
                    
                    routes[currNode][dst] = nextHop
                    nextHop = currNode
                    if nextHop == src:
                        break
            else:
                pass
    return routes
                    
        
def toXml(routes, bsNodeId, ipv6Prefix):
    nodeName = 'node'
    idName = 'id'
    rtName = 'rtentry'
    lenName = 'length'
    lenName2 = 'prefixLength'
    nhName = 'nextHop'
    prefixName = 'prefix'
    
    # hardware platform routing table format    
    root = ET.Element("root")
    
    # simulator routing table format
    root2 = ET.Element("root")
    
    # create BS node and outgoing link
    for route in routes:
        nodeStr = hex(int(route))
        currNode = ET.SubElement(root, nodeName, {idName:nodeStr, "ledset":"4"})
        
        for dest in routes[route]:
            prefix = ipv6Prefix + hex(int(dest)).split("0x")[1]
            len = "128"
            nhStr = ipv6Prefix + hex(int(routes[route][dest])).split("0x")[1]
            ET.SubElement(currNode, rtName, {prefixName:prefix, lenName:len, nhName:nhStr})
            
            ET.SubElement(root2, nodeName, {idName:nodeStr, prefixName:prefix, lenName2:len, nhName:nhStr})
    
    ET.ElementTree(root)
    ET.ElementTree(root2)
    return (root, root2)


main(sys.argv[1:])