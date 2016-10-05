from swig.cometos import *
import sys

class OmnetIniParser(TrafficGen):
	def __init__(self, filename, cfg):
		self.file = open(filename, 'r')
		
		
		
	def handleIncomming(self, ipAddr, srcPort, dstPort, data, len):
		try:
			print "received from " + self.addrToStr(ipAddr)
		except:
			print "Error in Python module:", sys.exc_info()[0]
			
		return;

		

	def addrToStr(self, addr):
		s = ""
		for i in range(8):
			s = s + hex(addr.getAddressPart(i))
			if (i < 7):
				s = s + "::"
		return s
