import sys;
import os;
import math;

def writeWrongUsage():
	print """
	wrong usage
	
	X Y
"""
	sys.exit()

numBranches = 0
numNodesBranch = 0

if (len(sys.argv) > 2):
	XDim = int(sys.argv[1], 10)
	YDim = int(sys.argv[2], 10)
else:
	writeWrongUsage()


foldername = "../cfg/Grid-" + str(XDim) + "x" + str(YDim) + "-Network/"

d = os.path.dirname(foldername)
if not os.path.exists(d):
	os.makedirs(d)
        
linkstatsfile = foldername + "linkStats.xml"
meshroutingfile = foldername + "meshUnderRouting.xml"
inifile = foldername + str(XDim) + "x" + str(YDim) + "-Network.ini"

linkstats = open(linkstatsfile, "w")
inistream = open(inifile, "w")
meshrouting = open(meshroutingfile, "w")

numNodes = XDim * YDim
xsize = (XDim * 20) + 100
ysize = (YDim * 20) + 100

print "creating " + inifile

inistream.write("""[Config Grid-""" + str(XDim) + """-""" + str(YDim) + """]
Network.numNodes = """ + str(numNodes) + """
**.node[*].nic.phy.linkStatsFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.node[*].sL.locationFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.connectionManager.connectionsFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.node[*].tg.routingFile = xmldoc(\"""" + foldername + """linkStats.xml")

**.playgroundSizeX = """ + str(xsize) + """m
**.playgroundSizeY = """ + str(ysize) + """m
**.playgroundSizeZ = 0m

""")


print "creating " + meshroutingfile

meshrouting.write("""<?xml version="1.0" ?>
<root>
""")

print "creating " + linkstatsfile

linkstats.write("""<?xml version="1.0" ?>
<root>
""")

simId = 0

for ynode in range(YDim):
	for xnode in range(XDim):
		nodeid = xnode + (ynode * 256)
		nodeidstr = "%X" % nodeid
		inistream.write("**.node[" + str(simId) + "].id = " + str(nodeid) + "\n")
		simId = simId + 1
		
		xpos = 50 + xnode * 20
		ypos = 50 + ynode * 20
		
		meshrouting.write("	<node id=\"" + nodeidstr + "\" nextHop=\"::\" prefix=\"64:29:30:31:0:0:0:0\" prefixLength=\"112\"/>\n")
		linkstats.write("	<node id=\"" + nodeidstr + "\" x=\"" + str(xpos) + "\" y=\"" + str(ypos) + "\">\n")
		
		for xneighbor in xrange(-3, +3):
			if (xnode + xneighbor) >= 0 and (xnode + xneighbor) < XDim:
				for yneighbor in xrange(-3, +3):
					if (ynode + yneighbor) >= 0 and (ynode + yneighbor) < YDim and not (yneighbor == 0 and xneighbor == 0):
						distance = math.sqrt(math.pow(xneighbor, 2) + math.pow(yneighbor, 2))
						rssiMean = 50 + (distance * 1.3)
						if rssiMean < 110: 
							neighbor = nodeid + xneighbor + (yneighbor * 256)
							neighborstr = "%X" % neighbor
							rssiVar = (0.15 * distance) + 0.3 #math.sqrt(distance) * 10
							linkstats.write("	   <link dest=\"" + neighborstr + "\" rssiMean=\"" + str(rssiMean) + "\" rssiVar=\"" + str(rssiVar) + "\" per=\"0\"/>\n")


		linkstats.write("	</node>\n\n")	
				
inistream.close()

linkstats.write("</root>\n")

linkstats.close()

meshrouting.write("</root>\n")

meshrouting.close()