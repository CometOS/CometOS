import os
import sys


startstring = "// Inserted by createShortNames.py | Do not edit or remove"
endstring = "// End of inserted by createShortNames.py | Do not edit or remove"

if len(sys.argv) < 3:
	print """Wrong usage:

	python createShortNames.py OUTDIR ROOTDIR FILE [INCLUDE_DIRS]
"""
	exit

file = sys.argv[3]
rootdir = sys.argv[2]
outdir = sys.argv[1]

mapfilename = os.path.join(outdir,"mapping.txt")
visitedFiles = []

def findFile(includeFile):
	if includeFile[0] == "\"" or includeFile == "<cometos.h>":
		fn = includeFile[1:len(includeFile)-1]
		if os.path.isfile(os.path.join(os.path.dirname(file), fn)):
			return os.path.join(os.path.dirname(file), fn)
		for i in range(4, len(sys.argv)):
			if os.path.isfile(os.path.join(sys.argv[i], fn)):
				return os.path.join(sys.argv[i], fn)
	return 0

def insertInFile(file):
	id = -1
	nextNr = 0
	
	visitedFiles.append(file)
	
	if os.path.isfile(mapfilename):
		mapping = open(mapfilename, "r")
		for line in mapping:
			lp = line.strip().split("\t")
			if len(lp) == 2:
				if lp[1] == file:
					id = int(lp[0])
				nextNr = int(lp[0]) + 1
		mapping.close()
	
	if id == -1:
		id = nextNr
		mapping = open(mapfilename, "a")
		mapping.write(str(id) + "\t" + file + "\n")
		mapping.close()
	
	(head, newfile) = os.path.split(file)
	(head, tail) = os.path.split(head)
	#while tail != "." and tail != "..": # old handling that does not support ROOT_DIR anywith apart from ../ or ./
	while head != rootdir:	
		newfile = os.path.join(tail, newfile)
		(head, tail) = os.path.split(head)
	newfile = os.path.join(tail, newfile) # not here in previous version (see above)
	newfile = os.path.join(outdir,newfile)
	(newfiledir, newfilename) = os.path.split(newfile)
	if not os.path.isdir(newfiledir):
		os.makedirs(newfiledir)
	
	writeToFile = 1
	if os.path.isfile(newfile):
		if os.path.getmtime(newfile) > os.path.getmtime(file):
			#print "no change in file " + file
			writeToFile = 0
	
	sfile = open(file, "r")
	if writeToFile == 1:
		wfile = open(newfile, "w")
	oldentry = 0
	lastinclude = 0
	linenum = 1
	for line in sfile:
		lineparts = line.strip().split(" ")
		if writeToFile == 1:
			if lastinclude == 1 and "#include" != lineparts[0]:
				wfile.write(startstring + "\n")
				wfile.write("#ifdef FILEID\n")
				wfile.write("#undef FILEID\n")
				wfile.write("#endif\n")
				wfile.write("#define FILEID	" + str(id) + "\n")
				wfile.write(endstring + "\n")
				wfile.write("#line " + str(linenum) + "\n")
			wfile.write(line)
		linenum += 1
		if lineparts[0] == "#include":
			incfile = findFile(lineparts[1])
			if incfile != 0 and not incfile in visitedFiles:
				insertInFile(incfile)
			lastinclude = 1
		else:
			lastinclude = 0
		if lineparts[0] == "#endif" and writeToFile == 1:
			wfile.write("#line " + str(linenum) + "\n")
		if lineparts[0] == "#else" and writeToFile == 1:
			wfile.write("#line " + str(linenum) + "\n")
	if writeToFile == 1:
		wfile.close()
	sfile.close()

if not os.path.isfile(file):
	print "Not a file"
else:
	insertInFile(file)
