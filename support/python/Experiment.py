from swig.cometos import *
import cometosMisc as lib
from stack.helpers import *
import sys
import time
import traceback


class UdpData():
	def __init__(self, modName, src, expected):
		self.receivedSeqs = []
		self.latencies = []
		self.src = src
		self.num = expected
		self.modName = modName
		
	def append(self, seq, latency):
		if seq not in self.receivedSeqs:
			self.receivedSeqs.append(seq)
			self.latencies.append(latency)
	
	def getLatency(self, idx):
		return latencies[idx]
	
	def received(self):
		return len(receivedSeqs)
	
	def buildStr(self, name, method):
		# format is module.metricName:srcNode=<value>
		return " " + str(self.modName) + "." + name + ":" + str(self.src) + "=" + str(method())
	
	def getAvgStr(self):
		stat = Statistics(self.latencies)
		
		s = ""
		s+= self.buildStr("Lat_avg", stat.getAvg)
		s+= self.buildStr("Lat_dev", stat.getStd)
		s+= self.buildStr("Lat_ConfInt", stat.getStdErr)
		s+= self.buildStr("Lat_min", stat.getMin)
		s+= self.buildStr("Lat_max", stat.getMax)
		s+= self.buildStr("Rcvd", stat.n)
		s+= " " + str(self.modName) + ".Num:" + str(self.src) + "=" + str(self.num)
		
		return s
	
	def __str__(self):
		s = ""
		for i in range(len(self.receivedSeqs)):
			s += "node=" + str(self.src) + " seq=" + str(self.receivedSeqs[i]) + " lat=" + str(self.latencies[i]) + " expected=" + str(self.num) + "\n"
		return s


class NodeEvent:
	''' 
	Wraps access to events among remote testbedNodes and the basestation itself
	For this to work, the (C++) class in question has to have a virtual 
	finished event which can be overwritten by the python wrapper.
	Takes a module and a remote event name and an event handler method. 
	In "remote" case, the event will be subscribed to and the event 
	passed through to the event handler method as soon as it fires. 
		
	'''
	def __init__(self, mod, name):
		self.mod = mod
		if isinstance(mod, RemoteModule):
			self.type = "remote"
			assert hasattr(self.mod, name)
			self.tf = getattr(self.mod, name)
		else:
			self.type = "local"		
			self.tf = name
			
		self.name = name

	def init(self, handler):
		self.handler = handler
		if self.type == "remote":
			return self.tf.subscribe(self.finishedEvent, 1)
		else:
			return mod.registerEventListener(self.tf, self.finishedEvent)
			
	
	def finishedEvent(self, id, obj):
		self.handler(id, obj)
			

class RemoteMethod():
	def __init__(self, modName, methodName):
		self.modName = modName
		self.methodName = methodName
	
	def invoke(self, node, *args):
		assert(hasattr(node, self.modName))
		mod = getattr(node, self.modName)
		assert(hasattr(mod, self.methodName))
		fun = getattr(mod, self.methodName)
		
		if isinstance(mod, RemoteModule):
			return remoteCall(self.modName, node.id, fun, *args)
		else:
			return fun(*args) 


class NodeCfgMethod(RemoteMethod):
	'''
	Encapsulates a remote call defined by module and method name.
	if a log is specified, the return value is put into this log.
	Which node to address is specified at invocation
	'''
	def __init__(self, cfgObj, modName, methodName, log=None):
		'''
		@param cfgObj     object to be passed to the remote method
		@param modName    name of the remote module
		@param methodName name of the remote method
		@param log        handle to a file to log the return value to
		'''
		self.obj = cfgObj
		self.modName = modName
		self.methodName = methodName
		self.log =log
		
	
	def getArgStr(self):
		return lib.getAttrString(self.obj)
	
	def invoke(self, node):
		if self.obj != None:
			result = RemoteMethod.invoke(self, node, self.obj)
		else:
			result = RemoteMethod.invoke(self, node)
		
		if self.log != None:
			self.log.write(time.strftime("%Y%m%d_%H%M%S") + " node=" + hex(node.id) + " mod=" +self.modName + " fun=" + self.methodName + " "+ lib.getAttrString(result) + "\n")
			self.log.flush()
		return result
	
class NwkCfg():
	'''
	Encapsulates calling of a remote method at a given set of nodes
	'''
	def __init__(self, cfg, nodes):
		''' 
		@param cfg    the remote method object to call
		@param nodes  a list of node objects 
		'''		
		self.runMethod = cfg
		self.nodes = nodes
		
		
	def prepare(self, resultMap=None):
		result = True
		self.missedNodes = []
		for node in self.nodes:
			ret = self.runMethod.invoke(node)
			if ret == None:
				self.missedNodes.append(node)
				result = False
			else:
				if resultMap != None:
					resultMap[node.id] = ret
				
		return result
				
	def getMissingList(self):
		return self.missedNodes
	
	def __str__(self):
		s = ""
		s += self.runMethod.modName + "." + self.runMethod.methodName + "(" + self.runMethod.getArgStr() + ")"
		return s + "\n"
	
	def __repr__(self):
		return self.__str__()


class Tss:
	def __init__(self, startMethod, stopMethod, nodes, masterNode):
		self.start = startMethod
		self.stop = stopMethod
		self.nodes = nodes
		self.masterNode = masterNode
		
	def prepare(self):
		result = True
		self.missedNodes = []
		for node in self.nodes:
			res = self.start.invoke(node)
			if res == False or res == None:
				self.missedNodes.append(node)
				print "failed"
				result = False
				
		print "Start Timesync at master (" + hex(int(self.masterNode.id)) + ")"
		self.start.obj.asMaster = True
		self.start.invoke(self.masterNode)
		
		# TODO possibly use event wait to wait for sync ack of all testbedNodes
		# for now, we wait 10 seconds and expect time sync to be finished
		time.sleep(30.0)  
		
		# for now, let TimeSync active as otherwise rather massive clock glitches occur
		
		return result
	
	def getMissingList(self):
		return self.missedNodes
	
	def __str__(self):
		s = ""
		s += self.start.modName + "." + self.start.methodName + "(" + self.start.getArgStr() + ")"
		s += "{0}; master={1}".format(self.nodes, self.masterNode)
		return s + "\n"
	def __repr__(self):
		return self.__str__()


class RunCfg():
	def __init__(self, runMethod, finiEvent, node):
		self.finiEvent = finiEvent			
		self.runMethod = runMethod
		self.node = node
		self.isValid = False
		
	def run(self):
		self.runMethod.invoke(self.node)
	
	
	def prepare(self, finiHandler):
		result = True
				
		if result:
			print "subscribing"
			result = self.finiEvent.init(finiHandler);
			
		self.isValid = result
		return result
	
	def offsetAdaption(self, last, rtt):
		if getattr(self.runMethod.obj, "offset") != None:
			print "decreasing offset by",
			if last != None:
				newOffset = last
			else:
				newOffset = self.runMethod.obj.offset
				
			if newOffset > rtt: 
				self.runMethod.obj.offset = newOffset - rtt
				print str(rtt),
			else:
				self.runMethod.obj.offset = 0
				print str(self.runMethod.obj.offset),
				
			print "to " + str(self.runMethod.obj.offset)
		
		return self.runMethod.obj.offset
	
	
	def isValid(self):
		return self.isValid
	
	def __str__(self):
		return "[{0}]@{1}\n".format(self.runMethod.getArgStr(), self.node.id);
	
	def __repr__(self):
		return self.__str__()

class UdpExperiment():
	def __init__(self, testbedNodes, preCfgs, paramcfgs, runCfgs, eventMod, postCfgs, collectCfgs, timeout, logHandle, runNo):
		self.preCfgs = preCfgs
		self.paramCfgs = paramcfgs
		self.runCfgs = runCfgs
		self.postCfgs = postCfgs
		self.collectCfgs = collectCfgs
		self.timeout = timeout / 1000
		self.log = logHandle
		self.runNo = runNo
		
		self.eventMod = eventMod
		assert(self.eventMod != None)
		self.eventMod.registerUdpListener(self.handleIncomming)
		
		self.eventHandlers = []
		self.udpData = {}
		self.finiEventData = {}
		self.transferFinishedVector = {}
		self.finiEvent = Event()
		self.collectedData = []
		self.targetIds = []
		self.testbedNodes = testbedNodes
			
	def cleanup(self):
		if (self.eventMod != None):
			self.eventMod.deregisterUdpListener(self.handleIncomming)
	
	
	def subscribeToEventSignal(self, eventHandler):
		self.eventHandlers.append(eventHandler)
		
	def unsubscribeFromEventSignal(self, eventHandler):
		if eventHandler in self.eventHandlers:
			self.eventHandlers.remove(eventHandler)
		
	
	def pre(self):
		if self.preCfgs != None:
			for cfg in self.preCfgs:
				print "Executing config " + str(cfg)
				if not cfg.prepare():
					print "PreConfig failed"
					return False
				else:
					print "PreConfig successful"
					pass
		
		if self.paramCfgs != None:
			for cfg in self.paramCfgs:
				print "Setting and storing parameters {0}".format(cfg)
				if not cfg.prepare():
					print "ParamConfig failed"
					return False
				else:
					print "Success"
					
		
		for cfg in self.runCfgs:
			if not cfg.prepare(self.transferFinished):
				print "initialization failed"
				return False
			else:
				print "successfully init " + hex(cfg.node.id)
				self.targetIds.append(cfg.node.id)
				
		for id in self.targetIds:
			## initialize UdpData with expected number of packets received,
			## which is retrieved from configuration object which is sent to
			## that node
			numExpected = self.getRunCfg(id).runMethod.obj.maxRuns
			self.udpData[id] = UdpData("tg", id, numExpected)
							
	
	def hasFinished(self):
		for cfg in self.runCfgs:
			if not cfg.hasFinished():
				return False
		return True 
				
	def run(self):
		last = None
		rtt = 0
		
		self.begin = getTimeMs()
		
		for cfg in self.runCfgs:
			if cfg.node.id in self.targetIds:
				last = cfg.offsetAdaption(last, rtt * 1000)
				print str(cfg) 
				(result, rtt) = lib.benchmark(cfg.run);
			else:
				print "skipping node " + str(cfg.node)
		
		print "--------------------------------------------------------------------------------"
		
		for eh in self.eventHandlers:
			eh("START")
		
		self.finiEvent.clear()
		if len(self.targetIds) == 0:
			self.finiEvent.wait(5.0)
		else:
			self.finiEvent.wait(self.timeout)
		
		
		if self.finiEvent.isSet():
			print "finished successfully"
		else:
			print "timeout"
		
		
		print "Collecting module data"
		if self.collectCfgs != None:
			for cfg in self.collectCfgs:
				print "Executing config " + str(cfg)
				resultMap = {}
				cfg.prepare(resultMap)
				self.collectedData.append(resultMap)
			
			
		
	def post(self, log):
		print "Executing post experiment actions: " + str(self.postCfgs)
		if self.postCfgs != None:
			for cfg in self.postCfgs:
				print "Executing config " + str(cfg)
				if not cfg.prepare():
					print "PostConfig failed"
					return False
				else:
					print "PostConfig successful"
					pass
		
		for key in self.udpData:
			print "Logging data for Node " + hex(key)
			log.write(str(self.udpData[key]))
		log.flush()
		pass
		
	
	def transferFinished(self, id, retVal):
		nodeId = int(id)
		hexNodeId = hex(nodeId)
		print "Received finished event from " + hexNodeId + " (" + hexNodeId + ")" 
#		print lib.getAttrString(retVal.udpStats) + "\n" + lib.getAttrString(retVal.lowpanStats)
		
		if not nodeId in self.transferFinishedVector:
			self.transferFinishedVector[nodeId] = True
			self.finiEventData[nodeId] = retVal  
#			print lib.getAttrString(self.finiEventData[nodeId].udpStats) + "\n" + lib.getAttrString(self.finiEventData[nodeId].lowpanStats)
		
		if len(self.transferFinishedVector) == len(self.targetIds):
			if not False in self.transferFinishedVector.values():
				self.finiEvent.set()
				
		return True
	
	
	def getRunCfg(self, nodeId):
		#print "getRunCfg|cfgs: " + str(self.runCfgs) + " nodeId: " + str(nodeId)
		for cfg in self.runCfgs:
			#print "cfg.node.id=" + str(cfg.node.id) + "(type=" + str(type(cfg.node.id)) + ")|nodeId=" + str(nodeId) + "(type=" + str(type(nodeId)) + ")|equal=" + str(cfg.node.id == nodeId)
			if cfg.node.id == nodeId:
				return cfg
		return None
	
	
	def getTimeSyncData(self, id):
		for cfg in self.runCfgs:
			if cfg.node.id == id:
				return cfg.timeSyncData
		return None
	
	
	def handleIncomming(self, id, seq, localTs, remoteTs):
		if not id in self.udpData:
			rcfg = self.getRunCfg(id)
			if (rcfg != None):
				numExpected = rcfg.runMethod.obj.maxRuns
				self.udpData[id] = UdpData("tg", id, numExpected)
			else:
				print "WARN: Received from node for which there is no runConfig object"
				return
		self.udpData[id].append(seq, localTs - remoteTs)
		
					
	def logData(self, base):
		# write down run number
		self.log.write("run={0}".format(self.runNo))
		# the following makes a strong distinction between bs and a node
		# which is okay for collection pattern but might suck for others
		for cfg in self.paramCfgs:
			self.log.write(cfg.runMethod.getArgStr())
		self.log.write("\n")
		
		# write log for basestation node
		try:
			self.log.write("NodeId=" + str(base.id))
			if self.getRunCfg(base.id) != None:
				self.log.write(self.getRunCfg(base.id).runMethod.getArgStr())
			self.log.write(lib.getAttrString(base.udp.getStats()))
			base.udp.resetStats()
			self.log.write(lib.getAttrString(base.low.getStats()))
			base.low.resetStats()
			for node in self.targetIds:
				if node in self.udpData:
					self.log.write(self.udpData[node].getAvgStr())
			self.log.write("\n")
		except:
			self.log.write(" ERR\n")
			print "Error occured logging BS: "
			traceback.print_exc()
			
		# write log for all other nodes 
		for node in self.testbedNodes:
			try:
				self.log.write("NodeId=" + str(node.id))
				rCfg = self.getRunCfg(node.id)
				if rCfg != None:
					self.log.write(rCfg.runMethod.getArgStr())	
				if (self.finiEventData.has_key(node.id)):
					self.log.write(lib.getAttrString(self.finiEventData[node.id].udpStats))
					self.log.write(lib.getAttrString(self.finiEventData[node.id].macStats))
					for i in range(8):
						self.log.write(" numPacketsWithRetries{0}={1}".format(i,deref(self.finiEventData[node.id].macStats.retryCounter.get(i))))
					
				for m in self.collectedData:
					if (m.has_key(node.id)):
						self.log.write(lib.getAttrString(m[node.id]))
						if type(m[node.id]) == cometos.LowpanOrderedForwardingStats:
							for i in range(5):
								self.log.write(" numBeUsed{0}={1}".format(i, deref16(m[node.id].beUsed.get(i))))
				
# 				self.log.write(lib.getAttrString(self.finiEventData[node].lowpanStats))
 				
				self.log.write("\n")
			except:
				self.log.write(" ERR\n")
				print "Error occured logging node info " + hex(node.id) +": "
				traceback.print_exc()
		self.log.flush()
		
