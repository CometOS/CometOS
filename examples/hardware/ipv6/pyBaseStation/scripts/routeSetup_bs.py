from modules.init_functions import *

# setup base station static routes 
bs.rm.add = bs.rm.addRoute
bs.rm.clear = bs.rm.clearRouting
bs.rm.clear()

prefix=IPV6_WSN_PREFIX + "0"
nextHop=IPV6_WSN_PREFIX + hex(BS_ATTACHED).split("0x")[1]

print "Set up BS routing: (prefix=" + prefix + "|nextHop=" + nextHop + "|prefixLen=112)"
setRoute(bs, prefix , 112, nextHop)