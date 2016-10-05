import swig.cometos as cometos
import sys
cometosDir = '../../../cometos/'
cometosPyPath = cometosDir + 'src/python/'

sys.path.append(cometosPyPath)
import cometosMisc as lib
import ast
import itertools
from ConfigParser import ParsingError
from debian.deb822 import OrderedSet

def readValue(s):
	if s[0] == "$":
		return s[1:]
	else:
		try:
			return ast.literal_eval(s)
		except:
			return s

def parse(file):
	''' Reads a config file a provides a list of lists of configuration
	objects encountered in the file. Each lists represents the combination
	of objects which represent the configuration for one run. They are
	ordered, so that not every new run all configuration objects have to 
	be rewritten. Constraints are already filtered out.
	'''
	parList = []
	valList = []
	calculations = {}
	with open(file, 'r') as f:
		for line in f:
			# print line.strip()
			if line[0] == "%":
				currClassString = line.strip()[1:]
			elif len(line.strip())==0 or line.strip()[0]=='#':
				continue
			else: 
				# actual parsing of attribute names and values
				s = line.split("=")
				expr = s[1].split()
				# print expr[0]
				element = readValue(expr[0])
				if len(expr) > 1:
					if not currClassString in calculations:
						calculations[currClassString] = {}
					for i in range(len(expr))[1:]:
						# print expr[i]
						expr[i] = readValue(expr[i])
					calculations[currClassString][s[0]] = expr[1:]
					
				if type(element)!=list:
					element = [element]
				parList.append((currClassString, s[0], 0))
				valList.append(element)
# 		print parList
# 		print valList
# 		print "cartesian product:"
# 		print list(itertools.product(*valList))
# 		print calculations
	return (parList, list(itertools.product(*valList)), calculations)


def getEvalString(parname):
	return 'getattr(cfgClassDict[c],"'+parname+'")'
		
def createObjects(parList, valList, calcs):
	
# 	print "Creating objects"
# 	print "================"
	objList = []
	cfgClasses = OrderedSet([obj[0] for obj in parList])
	cfgList = []
	
	# now iterate through configurations, create cfg objects and 
	# fill them with their corresponding values
	cfgIndex = 0
	for cfg in valList:
		newcfg = []
		cfgClassDict = {}
		
		# for each run configuration, create configuration objects
		for cfgClass in cfgClasses:
			cfgobj = getattr(cometos, cfgClass)()
			cfgClassDict[cfgClass] = cfgobj
			newcfg.append(cfgobj)
		
		# now iterate through all used parameters and set their values
		i = 0 
		for par in parList:
			c = par[0] # name of current config object
			param = par[1] # name of current parameter
			
			if c in calcs and param in calcs[c]:
				#print str(cfg[i]) + "|" + c + "|" + param
				if type(cfg[i]) == str:
					evalStr = getEvalString(cfg[i])
				else:
					evalStr = str(cfg[i])
				for e in calcs[c][param]:
					#print str(calcs[c][param]) + "|" + str(e) + "|" + str(type(e))
					if type(e) == str and hasattr(cfgClassDict[c], e):
						evalStr+= getEvalString(e)
					else:
						evalStr+= str(e)
				#print "evalStr=" + evalStr
				val = eval(evalStr)
			else:
				if type(cfg[i]) == str:
					val = eval(getEvalString(cfg[i]))
				else:
					val = cfg[i]
 			setattr(cfgClassDict[c], param, val)
			parList[i] = (par[0], par[1], par[2]+1) 
			i += 1
# 		print "run[{0}]: {1}".format(cfgIndex, [lib.getAttrString(cfgobj) for cfgobj in newcfg])
		cfgIndex += 1
		cfgList.append(newcfg)
	
	return cfgList
