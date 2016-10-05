#!/usr/bin/python
import re
import os.path
import sys
from subprocess import call
import shutil

class FileIdWriter:
    
    ASSERT_FUNCTION_NAME = "doAssert"
    ASSERT_DEFINE = "#define ASSERT(x)"
    DECLARATION_TYPE_STRING = "(uint16_t)|(int_16t)|(uint32_t)|(int32_t)|(int)|(unsigned int)"
    
    def __init__(self, mapfile):
        self.fileMap = {}
        self.mapfile = mapfile
        self.fId = 0
        
        if not os.path.isfile(mapfile):
            with open(mapfile, "w") as mf:
                pass
            
        with open(mapfile, "r") as mf:
            for line in mf:
                ## get ID FILENAME pair from file and add to map
                entry = line.split()
                if len(entry) == 2:
                    self.fileMap[entry[1].strip()] = int(entry[0])
                    self.fId = int(entry[0]) + 1
       
    def mapFilename(self, fname, mf):
        if fname not in self.fileMap:
            self.fileMap[fname] = self.fId
            mf.write("{0} {1}\n".format(self.fId, fname))
            self.fId += 1
                    


    def processFile(self, source, target):
        regexStrSingleLine = "{0}\(.*\)".format(FileIdWriter.ASSERT_FUNCTION_NAME)
        regexStrBegin = "{0}\(".format(FileIdWriter.ASSERT_FUNCTION_NAME)
        regexStrEnd = "\)"
        with open(source, 'r') as sf, open(target, 'w') as tf, open(self.mapfile, 'a') as mf:
            linebuf = []
            linebufReplacementIdx = -1
            for line in sf:
                #print line
                # parse files for occurrences of regex A:[0-9]+:
                matchObj = re.search(regexStrBegin, line)
                if matchObj:
                    singleLineMatchObj = re.search(regexStrSingleLine, line) 
                    if singleLineMatchObj:

                        pStart = singleLineMatchObj.start() + len(FileIdWriter.ASSERT_FUNCTION_NAME)
                        pEnd = singleLineMatchObj.end()
                        
                        assertParams = re.sub("[\(\)]", "", line[pStart:pEnd]).strip()
                        assertElements = assertParams.split(",")
                    else:
                        ## the doAssert call stretches over multiple lines; collect all lines
                        linebuf.append(line) 
                        continue
                    
                    ## check if this is a call to doAssert or the declaration (which must not be changed!)
                    if len(assertElements[0].split()) == 1 and not re.match(FileIdWriter.DECLARATION_TYPE_STRING, assertElements[0]):
                        # print "MATCH: {0}".format(assertElements)
                        fname = assertElements[1].strip()
                        self.mapFilename(fname, mf)
                        # now replace the arguments of doAssert by the original LINE and the replaced string
                        newline = "{0}({1}, {2}){3}".format(line[:pStart], assertElements[0], self.fileMap[fname], line[pEnd:])
#                         print("Replacing {0} by {1}".format(fname, self.fileMap[fname]))
#                         print("in line {0}".format(line))
                        tf.write(newline)
                    else:
                        ## this occurrence has to be a declaration --> use the original
                        # print "MATCH_DECL: {0}".format(line)
                        tf.write(line)
                elif len(linebuf) > 0:
                    ## we have recorded the beginning of a call to doAssert, collect remaining lines
                    if re.search(regexStrEnd, line):
                        assert linebufReplacementIdx != -1
                        linebuf.append(line)
                        # default format then seems to be: 0:begin line doAssert, 1:first param, 2:line directive, 3:',', 4:the file param, 5:line directive, 6:closing brackets
                        fname = linebuf[linebufReplacementIdx].strip()
                        self.mapFilename(fname, mf)
                        linebuf[linebufReplacementIdx] = "{0}\n".format(str(self.fileMap[fname]))
#                         print("Replacing {0} by {1}".format(fname, self.fileMap[fname]))
                        for l in linebuf:
                            tf.write(l)
                        linebuf=[]
                        linebufReplacementIdx = -1 
                    else:
                        linebuf.append(line)
                        if re.search("^\s*\"build", line):
                            linebufReplacementIdx = len(linebuf) - 1 
                else:
                    tf.write(line)
        
if __name__ == "__main__":
    if True: 
        #print "FILEIDWRITER:",
        ret = call(sys.argv[1:])
        if ret != 0:
            exit(ret)
        #print " ".join(sys.argv[1:])
        name = sys.argv[-1]
        #print(name)
        if name[-3:] == ".ii":
            #print(name)
            fw = FileIdWriter("mapping.txt")
            shutil.copyfile(name, name+".fid_before")
            fw.processFile(name, name + ".fid")
            shutil.copyfile(name+".fid", name)
    else:
        fw = FileIdWriter("map.test")
        fw.processFile("cc6kPnKB.ii.fid_before", "cc6kPnKB.ii.fid")
