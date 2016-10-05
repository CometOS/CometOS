import sys;
import os;
import math;
import random;
import time;

def writeWrongUsage():
	print """
	wrong usage
	
	NumNodes XSize YSize
"""
	sys.exit()

if (len(sys.argv) > 3):
	numNodes = int(sys.argv[1], 10)
	xsize = int(sys.argv[2], 10)
	ysize = int(sys.argv[3], 10)
else:
	writeWrongUsage()


foldername = "../cfg/Random-" + str(numNodes) + "-Network-" + str(time.time()) + "/"

d = os.path.dirname(foldername)
if not os.path.exists(d):
	os.makedirs(d)
        
linkstatsfile = foldername + "linkStats.xml"
meshroutingfile = foldername + "meshUnderRouting.xml"
inifile = foldername + "Random-" + str(numNodes) + "-Network.ini"

linkstats = open(linkstatsfile, "w")
meshrouting = open(meshroutingfile, "w")
inistream = open(inifile, "w")

print "creating " + inifile

inistream.write("""[Config Random-""" + str(numNodes) + """]
 """ + str(numNodes) + """
**.node[*].nic.phy.linkStatsFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.node[*].sL.locationFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.connectionManager.connectionsFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.node[*].tg.routingFile = xmldoc(\"""" + foldername + """linkStats.xml")

**.playgroundSizeX = """ + str(xsize) + """m
**.playgroundSizeY = """ + str(ysize) + """m
**.playgroundSizeZ = 0m

**.node[*].id = index

""")

inistream.close()

print "creating " + meshroutingfile

meshrouting.write("""<?xml version="1.0" ?>
<root>
""")

xpos = []
ypos = []

for node in range(numNodes):
	xpos.append(random.random() * xsize)
	ypos.append(random.random() * ysize)
	nodeidstr = "%X" % node
	meshrouting.write("	<node id=\"" + nodeidstr + "\" nextHop=\"::\" prefix=\"64:29:30:31:0:0:0:0\" prefixLength=\"112\"/>\n")
	
	
meshrouting.write("</root>\n")

meshrouting.close()


print "creating " + linkstatsfile

linkstats.write("""<?xml version="1.0" ?>
<root>
""")

random.seed()

simId = 0

for node in range(numNodes):
	
	nodeidstr = "%X" % node
	linkstats.write("	<node id=\"" + nodeidstr + "\" x=\"" + str(xpos[node]) + "\" y=\"" + str(ypos[node]) + "\">\n")
	
	for otherNode in range(numNodes):
		if node != otherNode:
			distance = math.sqrt(math.pow(xpos[node] - xpos[otherNode], 2) + math.pow(ypos[node] - ypos[otherNode], 2))
			rssiMean = 50 + (distance * 1.3)
			if rssiMean < 110: 
				neighborstr = "%X" % otherNode
				#rssiMean = 75 + (distance * 0.12)
				#rssiVar = math.sqrt(distance/100) * 10
				rssiVar = (0.15 * distance) + 0.3
				linkstats.write("	   <link dest=\"" + neighborstr + "\" rssiMean=\"" + str(rssiMean) + "\" rssiVar=\"" + str(rssiVar) + "\" per=\"0\"/>\n")


	linkstats.write("	</node>\n\n")	
				
linkstats.write("</root>\n")

linkstats.close()

