import time
from cometosMisc import *
from threading import Event
from fileinput import FileInput
from datetime import datetime
# from modules.cfg import *
# import modules.cfg
from stack.RemoteAccess import RemoteModule
import fileinput	 
import os
import math

REMOTE_RETRIES = 2

def clearDir(dir):
	print "clearing dir " + dir
	if os.path.exists(dir):
		for the_file in os.listdir(dir):     
			file_path = os.path.join(dir, the_file)    
			try:         
				if os.path.isfile(file_path):
					os.unlink(file_path)      
			except Exception, e:
				print e
	else:  
		os.makedirs(dir)


def remoteCall(moduleName, id, method, *args):
	global REMOTE_RETRIES
	retries = REMOTE_RETRIES
	while True:
		print "calling " + moduleName + "." + method.name + " at " + hex(id) + "  .....",
#		print "len=" + str(len(args)) + " str=" + str(*args)
		if len(args) != 0:
			obj = method(*args)
		else:
			obj = method()
		if obj != None:
			if not "real" in dir(obj) and "__getattribute__" in dir(obj):
				s = getAttrString(obj)
			else:
				s = str(obj)
			print("ok(" + s + ")")
			return obj
		elif retries == 0:
			print("failed overall")
			return None
		else:
			print("failed; retries left:" + str(retries))
			retries -= 1

def logStats(logHandle, moduleName, id, obj):
	global SEP
	assert obj
	
	if type(obj) == str:
		s = obj
	elif obj != None:
		# get string representing the attributes
		s = getAttrString(obj)
	else:
		s = "None"
	logHandle.write(str(datetime.now()) + SEP + hex(id) + SEP + moduleName + SEP + s +"\n")



def retrieveAndLogStats(logHandle, moduleName, node, methodName="getStats"):
	'''
	tries to get the stats for the given module via a getStats() method.
	If the given module is a remote module, an remote call is used, otherwise
	a local one. 
	If the stats have been retrieved successfully, the values of the stats
	object are logged according to the given attrList
	@param logHandle file descriptor of the logfile
	@param moduleName name of the module to get stats from
	@param node module which contains the stats-module (i.e. a node or base)
	'''
	time.sleep(0.1)
	mod = getattr(node, moduleName)
	func = getattr(mod, methodName)
	if isinstance(mod, RemoteModule):
		obj = remoteCall(moduleName, node.id, func)
		id = node.id
	else:
		obj = func()
		id = node.adr.getShortAddr()
		
	if (obj != None):
		logStats(logHandle, moduleName, id, obj)
	else:
		logStats(logHandle, moduleName, id, "E_TIMEOUT")
		
		


class Statistics:
	def __init__(self, values=[]):
		self.sum = 0
		self.sqrSum = 0
		self.len = 0
		self.lost = 0
		self.init = False
		self.values = {}
		self.min = None;
		self.max = None;
		
		for v in values:
			self.add(v)
		
	def add(self, value):
		if not self.init:
			assert(self.min == None)
			assert(self.max == None)
			self.min = value
			self.max = value
			self.init = True
			
		if self.min > value:
			self.min = value
			
		if self.max < value:
			self.max = value
		
		self.sum += value
		self.sqrSum += value * value
		self.values[self.len] = value
		self.len += 1
	
	def getMin(self): 
		return self.min
		
	def getMax(self):
		return self.max
	
	def missed(self):
		self.lost += 1
	
	def getAvg(self):
		if self.len <= 0:
			return None
		else:
			return (1.0 * self.sum) / self.len
	
	def getStd(self):
		if self.len <= 1:
			return 0.0
		else:
			avg = self.getAvg();
			# we had a wrong calculation here before, which took not 
			# 1/(n-1) as factor, i.e., ((1.0 * self.sqrSum) / self.len) - avg * avg
			return math.sqrt((1.0/(self.len - 1)) * (self.sqrSum - (1.0/self.len * self.sum * self.sum)));
	
	def getStdErr(self):
		if self.len <= 0:
			return None
		else:
			return self.getStd() / math.sqrt(self.len)
		
	def n(self): 
		return self.len
	 
	
