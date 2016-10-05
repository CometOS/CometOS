import sys;
import os;

def writeWrongUsage():
	print """
	wrong usage
	
	numBranches numNodesBranch
"""
	sys.exit()

numBranches = 0
numNodesBranch = 0

if (len(sys.argv) > 2):
	numBranches = int(sys.argv[1], 10)
	numNodesBranch = int(sys.argv[2], 10)
else:
	writeWrongUsage()


foldername = "../cfg/" + str(numBranches) + "-" + str(numNodesBranch) + "-Network/"

d = os.path.dirname(foldername)
if not os.path.exists(d):
	os.makedirs(d)
        
linkstatsfile = foldername + "linkStats.xml"
routingfile = foldername + "staticRouting.xml"
inifile = foldername + str(numBranches) + "-" + str(numNodesBranch) + "-Network.ini"

linkstats = open(linkstatsfile, "w")
routing	= open(routingfile, "w")
inistream = open(inifile, "w")

numNodes = numBranches * numNodesBranch
xsize = (numNodesBranch * 200) + 100
ysize = numBranches * 50 + 100

inistream.write("""[Config N""" + str(numBranches) + """-""" + str(numNodesBranch) + """]
Network.numNodes = """ + str(numNodes) + """
**.node[*].nic.phy.linkStatsFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.node[*].sL.locationFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.connectionManager.connectionsFile = xmldoc(\"""" + foldername + """linkStats.xml")
**.node[*].sr.routingFile = xmldoc(\"""" + foldername + """staticRouting.xml")
**.node[*].tg.routingFile = xmldoc(\"""" + foldername + """linkStats.xml")

**.playgroundSizeX = """ + str(xsize) + """m
**.playgroundSizeY = """ + str(ysize) + """m
**.playgroundSizeZ = 0m

""")

linkstats.write("""<?xml version="1.0" ?>
<root>
""")
routing.write("""<?xml version="1.0" encoding="UTF-8"?>
<root>
""")

simId = 0

for branch in range(numBranches):
	for node in range(numNodesBranch):
		nodeid = node + (branch * 256)
		nodeidstr = "%X" % nodeid
		inistream.write("**.node[" + str(simId) + "].id = " + str(nodeid) + "\n")
		simId = simId + 1
		if branch == 0:
			xpos = 50 + node * 100
			ypos = 50 + numBranches * 25
		else:
			xpos = 50 + numNodesBranch * 100 + node * 100
			xpos = branch * 50
		linkstats.write("	<node id=\"" + nodeidstr + "\" x=\"" + str(xpos) + "\" y=\"" + str(ypos) + "\">\n")
		if branch > 0:
			if node == 0:
				prev = numNodesBranch - 1
				prevstr = "%X" % prev
				linkstats.write("	   <link dest=\"" + prevstr + "\" rssiMean=\"50\" rssiVar=\"0\" per=\"0\"/>\n")					
		if node > 0:
			prev = nodeid - 1
			prevstr = "%X" % prev
			linkstats.write("	   <link dest=\"" + prevstr + "\" rssiMean=\"50\" rssiVar=\"0\" per=\"0\"/>\n")

		if nodeid > 0:
			preflen = 128
			k = 1
			while k < nodeid:
				k = k * 2
				preflen = preflen - 1
			routing.write("  <node id=\"" + nodeidstr + "\" prefix=\"64:29:30:31:0:0:0:0\"\n") 
			routing.write("  			   prefixLength=\"" + str(preflen) + "\"\n")
			routing.write("  			   nextHop=\"64:29:30:31:0:0:0:" + prevstr + "\" />\n")

		if node < (numNodesBranch - 1):
			next = nodeid + 1
			nextstr = "%X" % next
			linkstats.write("	   <link dest=\"" + nextstr + "\" rssiMean=\"50\" rssiVar=\"0\" per=\"0\"/>\n")

			routing.write("  <node id=\"" + nodeidstr + "\" prefix=\"64:29:30:31:0:0:0:0\"\n") 
			routing.write("  			   prefixLength=\"112\"\n")
			routing.write("  			   nextHop=\"64:29:30:31:0:0:0:" + nextstr + "\" />\n")

		if node == (numNodesBranch - 1):
			if branch == 0:
				for b in range(1, numBranches):
					next = b * 256
					nextstr = "%X" % next
					linkstats.write("	   <link dest=\"" + nextstr + "\" rssiMean=\"50\" rssiVar=\"0\" per=\"0\"/>\n")			
					
					routing.write("  <node id=\"" + nodeidstr + "\" prefix=\"64:29:30:31:0:0:0:" + nextstr + "\"\n") 
					routing.write("  			   prefixLength=\"120\"\n")
					routing.write("  			   nextHop=\"64:29:30:31:0:0:0:" + nextstr + "\" />\n")


		linkstats.write("	</node>\n\n")	
				
inistream.close()

linkstats.write("</root>\n")
routing.write("</root>\n")

linkstats.close()
routing.close()
