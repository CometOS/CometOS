from modules.helpers import *
from time import *

nodeList=[]

if len(args)==2:
	nodeList=nodes
	times=args[0]
	name = args[1]
elif len(args)==3:
	times=args[0]
	name=args[1]
	nodeList=args[2]
else:
	# default: use global node object (all nodes) and single execution
	nodeList=nodes
	times=1
	name="default_test"
	
print "Start pinging, config: times=" + str(times) + " name=" + name

if times >= 1:
	logName =  './log/ping/' + name + "/" + strftime("%Y%m%d") + "_" + strftime("%H%M%S")
	
#	dir = os.path.dirname(logName)
#	if not os.path.exists(dir):
#		os.makedirs(dir)
#	
#	log = open(logName, 'w')
	
	results = {}
	for node in nodeList: 
		results[node.id] = Statistics()
	
	for x in range(times):
		for node in nodeList:
			(res, latency) = benchmark(node.sys.ping)
			if res != None:
				print "pinged "+hex(node.id)+" in "+ str(latency)
				results[node.id].add(latency)
			else:
				print "timeout after " + str(latency) + "  at node " + hex(node.id)
			
			sleep(0.1)
#	for node in nodeList:
#		result = results[node.id]
#		if result.n() > 0:
#			s = hex(node.id) + SEP + str(result.n()) + SEP + str(times) + SEP + str(result.getAvg()) + SEP + str(result.getStd())
##			log.write(s + "\n")
#			print(s)
#		else:
##			log.write(hex(node.id) + SEP + "INF\n")