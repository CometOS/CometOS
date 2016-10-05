# createMain.py
#
# version: 1.4
#
# created on: 2012-09-17
# author: Martin Ringwelski
#

import sys;
from time import *

mainname = "main-created.cc"
statusled = 0

exchangefile = ""
leaveoutfile = ""

def writeWrongUsage():
	print """
	
Usage: python """ + sys.argv[0] + """ <NED-FILE> [[-el <FILE>] [-ll <FILE>] [-sled]] [<CC-FILE>]
	
Arguments:
  <NED-FILE>  NED file to be parsed

  -el <FILE>  File with list of modules to be exchanged with others
  -ll <FILE>  File with list of modules to be left out
  -sled       Lets one of the LEDs of the board blink while running
  
  <CC-FILE>   Name of the output C++ file. Default is "main-created.cc"

To get good results by this script, you should follow some rules.
The package name of the project should be the same as the name space. Header 
files should have the same name as the Modules. If a module can have several 
gates, it should be a template class with the first parameter being the number 
of gates.

The exchange file can be used to define modules in the simulation that need to 
be exchanged in the hardware implementation. Every line contains the name of the
module, which needs to be exchanged, and the new module name, separated with 
comma. Empty lines and lines beginning with "#" are ignored.

The leave-out file can be used to define modules in the simulation that are not 
needed in the hardware implementation. Every line contains the name of one 
module. Empty lines and lines beginning with "#" are ignored.

All those things and even more can also be set directly in the NED file. Just 
use comment lines in the submodules section of the following format:

// -leaveOut: ModuleName
// -exchange: ModuleName with OtherModuleName
// -setNamespace: NamespaceOf::ModuleName
// -setParameter: ModuleName: Param1, Param2, ..., ParamN
// -statusLED

The script also gives a suggestion about the content of the Makefile using the 
imported modules in the NED file. If the module namespaces resemble the folder
structure of your source code, this should work just fine. By adding comments
in the same line afterwards you can leave the modules out or exchange them with
other ones. For eg.:

import swig.cometos as cometos.auxiliary.TcpComm;	// changeTo cometos.platform.modules.SerialComm
import org.mixim.modules.phy.PhyLayer;	// onlySim

"""
	exit()

if (len(sys.argv) > 1):
    filename = sys.argv[1]
    if (len(sys.argv) > 2):
		n = 0
		for i in range(2, len(sys.argv)):
			if n == 0:
				if sys.argv[i][0] == "-":
					if sys.argv[i] == "-sled":
						statusled = 1
					elif sys.argv[i] == "-el" and i < len(sys.argv)-1:
						exchangefile = sys.argv[i+1]
						n = 1
					elif sys.argv[i] == "-ll" and i < len(sys.argv)-1:
						leaveoutfile = sys.argv[i+1]
						n = 1
					else:
						print "Argument number " + str(i) + " unknown: " + sys.argv[i]
						writeWrongUsage()
				elif i == (len(sys.argv)-1):
					mainname = sys.argv[i]
				else:
					print "Argument number " + str(i) + " unknown: " + sys.argv[i]
					writeWrongUsage()
			else:
				n = 0	
else:
	writeWrongUsage()
	
exchange = list()
if exchangefile != "":
	print "Reading exchange file " + exchangefile
	fobj = open(exchangefile, "r")
	for line in fobj:
		if line[0] != "#" and len(line.strip()) > 0:
			words = line.split(",")
			if len(words) == 2:
				exchange.append([words[0].strip(), words[1].strip()])
	fobj.close()
	
leaveout = list()
if leaveoutfile != "":
	print "Reading leave-out file " + leaveoutfile
	fobj = open(leaveoutfile, "r")
	for line in fobj:
		if line[0] != "#" and len(line.strip()) > 0:
			leaveout.append(line.strip())
	fobj.close()

namespaces = list()

noparam = list()
noparam.append("SerialComm")
noparam.append("Dispatcher")
noparam.append("SystemMonitor")
noparam.append("SimpleRouting")
noparam.append("LowpanDispatcher")
noparam.append("RemoteAccess")

comment = list()
includes = list()
prototypes = list()
wires = list()
parameterList = list()

status = 0
freeline = 0

makefileIncludes = list()
makefileSrcs = list()

# read in NED file
print "Reading NED File " + filename
fobj = open(filename, "r")
for line in fobj:
	comments = line.split("//")
	if len(comments) > 1:
		commentAfter = comments[1].strip()
	else:
		commentAfter = ""
	lineWOC = comments[0].strip()
	words = lineWOC.split(" ")

	if status == 0:
		words = line.split(" ")
		if words[0].strip() == "import":
# add include				
			if commentAfter != "onlySim":
				imp = words[1].strip().split(".")
				incl = imp[len(imp) - 1].split(";")[0]
				if incl != "*" and words[1].find("org.mixim") == -1:
					includes.append([incl,imp[0], freeline])
				freeline = 0
				
				commentParts = commentAfter.split(" ")
				if commentParts[0].strip() == "changeTo":
					imp = commentParts[1].strip().split(".")
					incl = imp[len(imp) - 1]
				
				includesLine=""
				srcLine=""
				includesLine="$("+imp[0].upper()+"_DIR)/src"
				srcLine="$("+imp[0].upper()+"_DIR)/src"
				for i in range(1,len(imp) - 1):
					if imp[i] == "omnetpp":
						includesLine=includesLine+"/platform"
						srcLine=srcLine+"/platform"
					else:	
						includesLine=includesLine+"/"+imp[i]
						srcLine=srcLine+"/"+imp[i]
				if incl != "*":
					srcLine=srcLine+"/"+incl+".cc"
				else:
					srcLine=""				
				if includesLine != "" and includesLine not in makefileIncludes:
					makefileIncludes.append(includesLine)
				if srcLine != "":
					makefileSrcs.append(srcLine)
		elif line.strip() == "submodules:":
			status = 1
			freeline = 0
		elif line.strip()[0:2] == "//":
			comment.append(line.strip())
			freeline = 0
		elif line.strip() == "":
			freeline = 1
	elif status == 1:
		words = line.split(":")
		if line.strip()[0:2] == "//":
			cline = line.strip()[2:].strip()
			parts = cline.split(":")
			if parts[0] == "-leaveOut":
				leaveout.append(parts[1].strip())
			elif parts[0] == "-exchange":
				ex = parts[1].split("with")
				exchange.append([ex[0].strip(), ex[1].strip()])
			elif parts[0] == "-setNamespace":
				namespaces.append([parts[1].strip(),parts[3].strip()])
			elif parts[0] == "-setParameter":
				ex = parts[2].split(",")
				parameterList.append([parts[1].strip(), ex])
			elif parts[0] == "-statusLED":
				statusled = 1
		elif line.strip() == "connections:":
			status = 2
			freeline = 0
		elif line.strip() == "":
			freeline = 1
		elif len(words) == 2:
# add prototype
			classname = words[1].split("{")[0].strip()
			instance = words[0].strip()
			shortname = instance
			if len(shortname) > 4:
				if any(c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ" for c in instance):
					shortname = instance[0]
					for c in instance:
						if c.isupper():
							shortname = shortname + c
					if len(shortname) > 4:
						shortname = shortname[0:4]
				else:
					shortname = instance[0:4]
			prototypes.append([classname, instance, shortname, 0, freeline])
			freeline = 0
	elif status == 2:
		words = line.split(" --> ")
		if len(words) == 2:
# add wire
			cfrom = words[0].strip()
			if cfrom.find("[") != -1:
				inst = cfrom.split(".")[0].strip()
				parta = cfrom[0:cfrom.find("[")]
				partb = cfrom[cfrom.find("[")+1:cfrom.find("]")]
				cfrom = parta + ".get(" + partb + ")"
				for p in prototypes:
					if p[1] == inst and p[3] <= int(partb):
						p[3] = int(partb) + 1
			cto = words[1].split(";")[0].strip()
			if cto.find("[") != -1:
				parta = cto[0:cto.find("[")]
				partb = cto[cto.find("[")+1:cto.find("]")]
				cto = parta + ".get(" + partb + ")"
				for p in prototypes:
					if p[1] == inst and p[3] <= int(partb):
						p[3] = int(partb) + 1
			wires.append([cfrom, cto, freeline])
			freeline = 0
		elif line.strip() == "":
			freeline = 1
fobj.close()

# creating main.cc
print "Writing C++ File " + mainname
mnf = open(mainname, "w")
lt = localtime()
y, m, d = lt[0:3]
year = str(y)
month = str(m).zfill(2)
day = str(d).zfill(2)

mnf.write("""/*
 * """+mainname+"""
 *
 * Created by the 'createMain.py' script. Please check the namespaces and 
 * instantiation of template classes.
 *
 * created on: """ + year + "-" + month + "-" + day + """
 *
 */

""")

for c in comment:
	mnf.write(c + "\n")
if len(comment) > 0:
	mnf.write("\n")

mnf.write("""/*INCLUDES-------------------------------------------------------------------*/
 
#include "cometos.h"
 
""")

if statusled == 1:
	mnf.write("#include \"palLed.h\"\n\n")

# writing includes
todelete = list()
for i in includes:
	used = 0
	infile = i[0]
	for p in prototypes:
		if p[0] == infile:
			used = 1
	if used == 1 and infile not in leaveout:
		for e in exchange:
			if infile == e[0]:
				mnf.write("// Exchanged " + infile + " with " + e[1] + "\n")
				infile = e[1]
		if i[2] == 1:
			mnf.write("\n")
		mnf.write("#include \"" + infile + ".h\"\n")
	else:
		todelete.append(i)

if len(todelete) > 0:
	mnf.write("\n/* --- Left Out Includes --- */\n")
	for t in todelete:
		mnf.write("//#include \"" + t[0] + ".h\"\n")
		includes.remove(t)
		
mnf.write("""
using namespace cometos;

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

/*PROTOTYPES-----------------------------------------------------------------*/

""")

# writing prototypes
todelete = list()
for p in prototypes:
	included = 0
	cn = p[0]
	for e in exchange:
		if cn == e[0]:
			cn = e[1]
	cname = cn
	for i in includes:
		if i[0] == p[0]:
			included = 1
			for namespaceModule in namespaces:
				if cname == namespaceModule[1]:
					i[1] = namespaceModule[0]
			if i[1] != "cometos":
				cname = i[1] + "::" + cname;
	if included == 1:
		if p[4] == 1:
			mnf.write("\n")
		mnf.write("static " + cname)
		if p[3] > 0:
			mnf.write("<" + str(p[3]) + ">")
		mnf.write(" " + p[1])
		if cn not in noparam:
			defaultParameter = 1
			for paramEntry in parameterList:
				if cn == paramEntry[0]:
					defaultParameter = 0
					nParam = 0
					for param in paramEntry[1]:
						if nParam == 0 and param.strip() != "":
							mnf.write("(")
							nParam = 1;
						else:
							mnf.write(", ")
						mnf.write(param.strip());
					if nParam > 0:
						mnf.write(")")
			if defaultParameter == 1:
				mnf.write("(\"" + p[2] + "\")")			
		mnf.write(";\n")
	else:
		todelete.append(p)

if len(todelete) > 0:
	mnf.write("\n/* --- Left Out Prototypes --- */\n")
	for t in todelete:
		mnf.write("//static " + t[0])
		if t[3] > 0:
			mnf.write("<" + str(t[3]) + ">")
		mnf.write(" " + t[1] + "(\"" + t[2] + "\");\n")
		prototypes.remove(t)

if statusled == 1:
	mnf.write("""
/* Using this as status LED*/
void toggleLed();

SimpleTask task(toggleLed);
TaskScheduler scheduler;

void toggleLed() {
	palLed_toggle(1);
	getScheduler().add(task, 250);
}
""")

mnf.write("""
int main() {

/*WIRING---------------------------------------------------------------------*/

""")

# writing wiring
todelete = list()
for w in wires:
	used = 0
	for p in prototypes:
		if p[1] == w[0].split(".")[0] or p[1] == w[1].split(".")[0]:
			used = used + 1
	if used == 2:
		if w[2] == 1:
			mnf.write("\n")
		mnf.write("\t" + w[0] + ".connectTo(" + w[1] + ");\n")
	else:
		todelete.append(w)

if len(todelete) > 0:
	mnf.write("\n/* --- Unused wires --- */\n")
	for t in todelete:
		mnf.write("//\t" + t[0] + ".connectTo(" + t[1] + ");\n")
		wires.remove(t)


mnf.write("""
/*INITIALIZATION-------------------------------------------------------------*/

	initialize();
	
""")

if statusled == 1:
	mnf.write("\tgetScheduler().add(task, 250);\n")
	
mnf.write("""		
/*START----------------------------------------------------------------------*/

	run();
	
	return 0;
}
""")

mnf.close()

print """
Suggested Makefile content:
SRCS_CXX += """+mainname+"""
"""

for includes in makefileIncludes:
	print "INCLUDES += " + includes

print "\n"

for srcs in makefileSrcs:
	print "SRCS_CXX += " + srcs

print """
# include board-specific makefiles
include $(COMETOS_DIR)/apps/Makefile.board"""
