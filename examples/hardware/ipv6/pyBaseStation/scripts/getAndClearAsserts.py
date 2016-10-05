from modules.helpers import *
from time import *


for n in nodes:
	a = n.sys.ai()
	if a != None:
		print "FileID=" + str(a.fileId) + " Line=" + str(a.line)
	n.sys.ac()