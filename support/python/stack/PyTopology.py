from swig.cometos import *
import cometosMisc as lib
from helpers import *
import sys
import time
import math
import random

from Experiment import *
from random import normalvariate

class Topology:
	def __init__(self, name, nodes):
		self.nodes = {}
		self.numTxs = {}
		self.numMinValues = {}
		self.name = name
		self.targets = nodes


	def add(self, node, neighbor,  sums):
		if not node in self.nodes:
			self.nodes[node] = {}
		self.nodes[node][neighbor] = sums
	
	def test(self, mean, std):
		dummy = SumsMinMaxRssi32()
		reference = SumsMinMaxRssi32()
		missed = 0
		minVals = 0
		for i in range(1000):
			val = normalvariate(mean, std)
			print str(val)
			reference.add(val)
			print str(reference.n())
			if (val <= -100):
				missed += 1
				pass
			elif val <= -90:
				dummy.add(-90)
				minVals += 1
			else:
				dummy.add(val)
		
		print "sum=" + str(dummy.getSum()) + " sqrSum=" + str(dummy.getSqrSum()) + " n=" + str(dummy.n())
		
		(avg, var) = Topology.postProcess(self, reference, 1000)
				
		print "mu=" + str(avg) + " var=" + str(var)
		print ""
		
		(avg, var) = self.postProcess(dummy,  missed+dummy.n());
		
		print "mu=" + str(avg) + " var=" + str(var)
		
		print "" 
		print ""
		
	
	''' Standard post-processing, platform independent --- take 
    sums and calculate average and variance and ignore missed packets '''
	def postProcess(self, sum, sent):
		print "0",
		return self.getAvgAndVarFromSums(sum.n(), sum.getSum(), sum.getSqrSum())
	
	
	def getAvgAndVarFromSums(self, n, sum, sqrSum):
		if n > 1:
			avg = sum * 1.0 / n
			var = (1.0/(n-1))*(sqrSum * 1.0 - ((sum * sum) * 1.0 / n))
		else:		
			avg = 0
			var = 0
			
		return (avg, var)

	def getValues(self):
		for n in self.targets:
			print "Retrieving numSent from " + str(n.id) + " ... ",
			numTx = n.tm.gns()
			self.numTxs[n.id] = numTx;
			if None != numTx:
				print "ok, numSent=" + str(numTx)
			else:
				print "failed"
				return
			
			print "Retrieving neighborlist of " + str(n.id) + " ... ",
			nblist = n.tm.gnl()
			if None != nblist:
				print "ok, " + str(nblist.size())+  " neighbors found"
			else:
				print "failed"
				return
			
			if nblist != None:
				it = nblist.begin()
				while it != nblist.end():
					nb = nblist.get(it)
					print "Retrieving info for neighbor  " + str(nb.id) + " ... ",
					sum = n.tm.gi(nb)
					if sum != None:
						print "ok"
						self.add(n.id, nb.id, sum)
					else:
						print "failed"
					it = nblist.next(it)
					
						
			else:
				print "List retrieval failed"
				return
	
	def cmd(self, mName, val):
		m = NodeCfgMethod(val, self.name, mName)
		nwk = NwkCfg(m, self.targets)
		nwk.prepare()
	
	def start(self, interval, size):
		self.cmd("start",  cometos.TmConfig(interval, size))
	
	def stop(self):
		self.cmd("stop", None)
				
	def clear(self):
		self.cmd("clear", None)
		self.nodes.clear()
	
	
	def log(self, filename, printHexa=False):
 		logfile = open(filename, 'w')
 		logfile.write("dst src rssiAvg rssiVar numRcvd etx\n")
		for n in self.nodes:
			for nb in self.nodes[n]:
				sum = self.nodes[n][nb]
				sent = self.numTxs[nb]
				
				print "node=" + str(n) + " neighbor=" + str(nb)
				(avg, var) = self.postProcess(sum, sent);
				print "avg= " + str(avg) + " var=" + str(var)
				print ""

 				if printHexa:
 					toStr = hex
 				else:
 					toStr = str
 				if not (avg == 0 and var == 0):				
					logfile.write(toStr(n) + " " + toStr(nb) + " " + str(avg) + " " + str(var) + " " + str(sum.n()) + " " + str(sent * 1.0 / sum.n()) + "\n")
				
	def __str__(self):
		s = ""
		for n in self.nodes:
			s += hex(n) + "\n"
			s += "-----------------------------------\n"
			for nb in self.nodes[n]:
				sum = self.nodes[n][nb]
				if sum.n() != 0:
					avg = sum.getSum() * 1.0 / sum.n()
					var = sum.getSqrSum() * 1.0 - ((sum.getSum() * sum.getSum()) / sum.n())
				else:
					avg = 0
					var = 0
				s +=  hex(nb) + ": " + str(avg) + " dBm " + "(var=" + str(var) + ") from " + str(sum.n()) + " values \n";
			s+="\n\n"
			
		return s



class AvrTopology(Topology):
	def __init__(self, name, nodes, rmin=-90, rloss=-102):
		#super(AvrTopology, self).__init__(name, nodes)
		Topology.__init__(self, name, nodes)
		self.erfMap = {}
		self.rmin = rmin
		self.rloss = rloss
				
	def inverf(self, y):
		x = 0
		xb = y
		i = 0
		while (x != xb and i < 200):
			x = xb
			xb = x - ( ( math.erf(x) - y ) / ( ( 2 / math.sqrt(math.pi) ) * math.exp(-(x * x)) ) )
			i = i + 1
		return xb

	
	def postProcess(self, sum, sent):
		rmin = self.rmin
		rloss = self.rloss
		
		# we try to fit a normal distribution to the retrieved results
		# for this, we consider the 3 buckets 
		#           RSSI > -90 dBm  (all packets with RSSI better than -90 dBm)
		# -90 dBm > RSSI > -100 dBm (packet which were received but below 
		#                           -90 dBm and are reported on AVR as -90)
		# -100 dBm> RSSI            (we assume that packets which were lost had
		#                            RSSI below -100 dBm
		# The found normal distribution is used to fill in the missing 
		# values from -90 and lower -- those are then added to original 
		# sum after it was corrected (remove -90 values) 
		assert(sum.getNumMinValues() <= sum.n())
		missed = sent - sum.n()
		regular = sum.n() - sum.getNumMinValues()
		nummin = sum.getNumMinValues()

		print "missed=" + str(missed) + " regular=" + str(regular) + " nummin=" + str(nummin) + " min=" + str(sum.getMin())
		
		
		print " m/s=" + str(1.0* missed/sent) + "r/s= " + str(1.0* regular/sent) + "n/s= " + str(1.0 * nummin/sent)
		val1 = 2.0 * regular/sent - 1
		x1 = self.inverf(val1)
		assert(math.fabs(math.erf(x1) - val1) < 0.0000001)
		
		val21 = (2.0 * sum.n()) / sent -1
		x21 = self.inverf(val21)
		assert(math.fabs(math.erf(x21) - val21) < 0.0000001)
		
		val23 = (1 - 2.0 * (nummin + missed)/sent)
		x23 = self.inverf(val23)
		
		val3 = 1 - 2.0 * missed/sent
		x3 = self.inverf(val3)
		assert(math.fabs(math.erf(x3) - val3) < 0.0000001)
		
		
		if (((1.0*nummin)/sent < 0.005) and ((1.0*missed)/sent < 0.005) or (sum.getMin() > -90)):
			# nummin and missed are really small, ignore them and get avg/var
			# from summed up values
			(mu, var) = Topology.postProcess(self, sum, sent)
		elif regular == 0 and nummin == 0:
			assert(False)	
			pass		
		else:
			if regular >= missed and regular >= nummin:
				# regular largest, who's second?
				if missed > nummin:
					use = 13
				else:
					use = 12
			elif missed >= regular and missed >= nummin:
				if regular > nummin:
					use = 13
				else:
					use = 23
			else:
				if regular > missed:
					use = 12
				else:
					use = 23
					
			# now actually calculate distribution
			if use == 12:
				print "12"
				# x1 and x21, regular and cropped packets
	 			mu = -1.0 * x21 * (rmin - rloss) / (x1 - x21) + rloss
	 			var = 0.5 * ((rmin - rloss)/(x1 - x21)) * ((rmin - rloss) / (x1 - x21)) 
			if use == 13:
				print "13"
				# x1 and x3, regular and missed packets
	 			mu = -1.0 * x1 * (rmin - rloss)/(x1 - x3) + rmin
	 			var = 0.5 * ((rmin - rloss) / (x1 - x3)) * ((rmin - rloss) / (x1 - x3)) 	
			if use == 23:
				print "23"
				mu = -1.0 * x23 * (rmin - rloss)/(x23 - x3) + rmin
				var = 0.5 * ((rmin - rloss) / (x23 - x3)) * ((rmin - rloss) / (x23 - x3))
		
			# now we got an estimate of a normal distribution which 
			# we use to derive the values which were not recorded
			std = math.sqrt(var)
			
			
			
			correctedSum = sum.getSum() - rmin * nummin
			correctedSqrSum = sum.getSqrSum() - rmin*rmin*nummin
			print "sum=" + str(sum.getSum()) + "|" + str(correctedSum) + " sqrSum=" + str(sum.getSqrSum()) + "|" + str(correctedSqrSum)
			postAdded = 0
			for i in range(missed+nummin):
				while True:
					val = normalvariate(mu, std)
					if val < rmin and val > rloss - 20:
						correctedSum += val
						correctedSqrSum += val*val
						postAdded += 1
						break
			assert(sent == regular + postAdded)		
			
			
			(mu, var) = self.getAvgAndVarFromSums(sent, correctedSum, correctedSqrSum)
			assert(var < 250)
		return (mu, var)

			
