import sys
#from xml.etree.ElementTree import *
import xml.etree.ElementTree as ET
from xml.dom.minidom import parseString


def getNodeSet():
	
	return ["2218", "2217", "2216", "2203", "2201", "2200", "21FD", "21FC"]
#	return ["221B", "221A", "2219", "2218", "2217", "2216"]


def prettyPrint(element):
    txt = ET.tostring(element)
    return parseString(txt).toprettyxml()



if len(sys.argv) != 3:
	print "usage:" + str(sys.argv[0]) + " <inFile> <outFile>"
	quit()


output = open(sys.argv[2], 'w')

tree = ET.ElementTree()
tree.parse(sys.argv[1])
xmlNodes = list(tree.iter("node"))

entries = {}
mapping = {}


nodeset = getNodeSet()

for xmlNode in xmlNodes:
	currId = int(xmlNode.attrib["id"])
	if not currId in entries:
		entries[currId] = []
		
		# ignore 0 which is supposed to be the basestation 
		if currId != 0: 
			mapping[currId] = nodeset.pop()
		else:
			mapping[currId] = "0"
		
		print str(currId) +  " " + mapping[currId]
	if currId != 0:
		print str(currId) + " " + str((xmlNode.attrib["prefix"], xmlNode.attrib["prefixLength"], xmlNode.attrib["nextHop"]))
		entries[currId].append((xmlNode.attrib["prefix"], xmlNode.attrib["prefixLength"], xmlNode.attrib["nextHop"]))
		
	
root = ET.Element("root")


print ""
print "generating new xml document..."
print "------------------------------"

for node in entries:
	if node == 0:
		continue
	currNode = ET.SubElement(root, 'node', {"id":"0x"+mapping[node]})# id="0x' + nodename + '"')
	for entry in entries[node]:
		print str(node) + " " + str(entry)
		prefixList = entry[0].split(':')
		assert len(prefixList) == 8
		prefixList[7] = mapping[int(prefixList[7])]
		
		nextHopList = entry[2].split(':')
		assert len(nextHopList) == 8
		nextHopList[7] = mapping[int(nextHopList[7])]
		
		prefixStr = ":".join(prefixList)
		nextHopStr = ":".join(nextHopList)
		rtData = ET.SubElement(currNode, 'rtentry',  {"prefix" : prefixStr, "length" : entry[1], "nextHop" : nextHopStr})
	
tree = ET.ElementTree(root)
output.write(prettyPrint(root))


