# Test for serialization of lists

from random import Random

r=Random(1);
count=0

while True:
	for i in node:
		count=count+1
		size=r.randint(0, 19)
		start=r.randint(0, 255-20)
		print "Call node "+str(i)+" with "+str(size)+" "+str(start)+" ("+str(count)+")"
			
		val=node[i].traffic.get(size,start)
		
		# check data
		for x in range(size):
			assert (start+x)==cometos.getTrafficPayload(val,x)