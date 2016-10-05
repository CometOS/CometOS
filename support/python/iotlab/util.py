import sys
import traceback
import json
import subprocess
import os
import time
import traceback
import datetime 
import xml.etree.ElementTree as ET
from xml.dom.minidom import parseString

### iotlab imports
import json

def nodeJsonFileToNodeXmlFile(jsonfile, xmlfile):
    ''' transform a iotlab-formatted (json) node file into a xml file
        usable for the average cometos basestation (default etc.). Note:
        obviously, no routes will be installed here!
    '''
    nodes = parseNodeFile(jsonfile)
    root = ET.Element("root")
    for n in nodes:
        currNode = ET.SubElement(root, 'node', {'id':hex(n['uid']), "ledset":"4", 'iotlabId':str(n['id'])})
    ET.ElementTree(root)
    with open(xmlfile, "w") as xf:
        s = ET.tostring(root)
        xf.write(parseString(s).toprettyxml())



def nodeStringToNodeJsonFile(s, nodefile, targetfile, arch, site, append):
    ''' Use a node string, e.g., as retrieved from the graphical web tool
        and transfer all matching node records from the site-wide file to
        the target file. Useful when using the web gui tool to select
        nodes for an experiment.
        Accepts, e.g. <NodeId>+<NodeId>-<NodeId>, ranges delimited with "-"
        must appear as pair, e.g., not <NodeId>-<NodeId>-<NodeId> is not allowed
    '''
    nodes=[]
    nodesTmp = s.split("+")
    for n in nodesTmp:
        r = n.split("-")
        if len(r) == 1:
            nodes.append(n)
        elif len(r) == 1:
            rangeNums = r.split("-")
            nodes.extend(range(rangeNums[0], rangeNums[1]+1))
        else:
            print "invalid range expression: {0}".format(n)
    print "list of nodes to copy: {0}".format(nodes)
    nodes = [int(n) for n in nodes]
    try:
        with open(nodefile, 'r') as sf, open(targetfile, 'w') as tf:
            sfNodes = json.load(sf)
            if append:
                tfNodes = json.load(tf)
            else:
                tfNodes = {"items" : []}
            for item in sfNodes['items']:
                (uid, nwkdAddr, serialHost, nodeId) = parseNodeItem(item)
                if nodeId in nodes:
                    if item['archi'] == arch and item['site'] == site:
                        print "Copy {0} to new file".format(nodeId)
                        tfNodes['items'].append(item)
            print "Copying {0} records; node string contained {1} nodes".format(len(tfNodes['items']), len(nodes))
            json.dump(tfNodes, tf, indent=4)
    except Exception as e:
        print "Exception opening files {0}, {1}; aborting".format(nodefile, targetfile)
        traceback.print_exception(Exception, e, sys.exc_info()[2])
    
    

def nodeJsonFileToNodeString(filename):
    ''' From a JSON file, extract node's short names a prepare
        a string that can be used as a node string in the iotlab web interface
        or the CLI command line tools, e.g., 177+179+195
    '''
    nodes = parseNodeFile(filename)
    s = "+".join([str(elem['id']) for elem in nodes])
    return s
   
def parseNodeItem(nodeDict):
     ## get node UID, which is a hexadecimal number
    if nodeDict["uid"].strip() == "":
        uid = None
    else:
        uid = int(nodeDict["uid"], 16)
    
    ## get full nwk address
    nwkAddr = nodeDict["network_address"]
     
    ## get first part of nwk address, which is the node identifier, e.g. m3-177
    serialHost = nwkAddr.split(".")[0]
     
    ## get node identifier
    nodeId = int(serialHost.split("-")[1])
    
    pos = { "x" : float(nodeDict['x']), "y" : float(nodeDict['y']), "z" : float(nodeDict['z'])}
    
    return (uid, nwkAddr, serialHost, nodeId, pos) 


def parseNodeFile(filename):
    nodes = []
    jsonnodes = []
    try:
        with open(filename, "r") as nf:
            jsonnodes = json.load(nf)
    except IOError:
        print("File {0} not found; returning empty node object".format(filename))
    
    ### file not read, return empty object
    if jsonnodes == []:
        return nodes
    
    for item in jsonnodes["items"]:
        (uid, nwkAddr, serialHost, nodeId, pos) = parseNodeItem(item)
        nodes.append({"uid" : uid, "id" : nodeId, "serialHost":serialHost, "nwkAddr" : nwkAddr, "pos": pos})
    return nodes


