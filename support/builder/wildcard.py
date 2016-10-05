import os, fnmatch, re

def all_files(directory, pattern):
    #print "calling all_files({0}, {1})".format(directory, pattern)
    for root, dirs, files in os.walk(directory, followlinks=True):
        #print "{0} {1} {2}".format(root, dirs, files)
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                # print filename
                ## strip cometos_path and location of this particular dir
                ## and return only relative local path
                yield re.sub("{0}/*".format(directory), "", filename)
