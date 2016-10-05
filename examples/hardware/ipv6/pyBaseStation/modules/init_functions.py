import swig.cometos as cometos 

def setRoute(node, prefix, length, nextHop):
	node.rm.add(cometos.RouteInfo(
									cometos.IPv6Address(prefix), 
									length, 
									cometos.IPv6Address(nextHop)))

def storeRoute(node, prefix, length, nextHop, pos):
	node.rm.sr(cometos.RouteInfo(
									cometos.IPv6Address(prefix), 
									length, 
									cometos.IPv6Address(nextHop)),
			                        pos)

def setNode(node, led, routes, storeRoutes):
	print "Clear Routing Table"
	
	# if we use persistency, also clear storage first
	if storeRoutes:
		node.rm.cs()
	
	node.rm.clear()
	
	# use persistent storing or memory structure
	print "Setting Routing Table"
		
	i = 0
	for route in routes:
		print "Prefix " + route.attrib["prefix"] + "/" + route.attrib["length"] + " nextHop: " + route.attrib["nextHop"]
		if storeRoutes:
			storeRoute(node, 
				route.attrib["prefix"], 
				int(route.attrib["length"], 0), 
				route.attrib["nextHop"],
				i)
		else:
			setRoute(node, 
				route.attrib["prefix"], 
				int(route.attrib["length"], 0), 
				route.attrib["nextHop"])
		i += 1

def setInitialValues(nodes, storeRoutes):
#	i = int(0)
	for node in nodes:
		print "Setting Node " + hex(node.id)
		setNode(node, node.led, node.rtEntries, storeRoutes)
	
	

#setInitialValues()
					
def sendEcho(node, dest, size):
	node.mt.echo(cometos.SendingInfo(dest.id, size))

def sendPacket(node, dest, size):
	node.mt.udp(cometos.SendingInfo(dest.id, size))