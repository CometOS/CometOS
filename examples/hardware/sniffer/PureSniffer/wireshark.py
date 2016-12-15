#!/usr/bin/env python
from scapy.all import *
import time
import subprocess
import fileinput
import os
import re

FIFONAME = '/tmp/802154fifo'

try:
    os.remove(FIFONAME)
except:
    pass # not existing so no problem

os.mkfifo(FIFONAME)
subprocess.call("wireshark -k -i %s &"%FIFONAME, shell=True)

class IEEE802154(Packet):
    name = "802.15.4"
    fields_desc=[ StrLenField("data", "", "len") ]

bind_layers( IEEE802154, LLC, type=2 )
conf.l2types.register(195, IEEE802154)

fdesc = PcapWriter(FIFONAME);

for line in fileinput.input():
    data = ""
    #try:
    if True:
        if "m3" in line:
            # Probably IotLab log
            # TODO parse time
            #m = re.search(';(.*)',line)
            m = re.search(";.*;(.*)",line)
            if m:
                line = m.group(1)
                #print line
    print line
            
    data = ''.join(map(lambda x: chr(int(x,16)), filter(lambda x: "0x" in x, line.split(" "))))
    #except:
    #    pass # could not decode

    if data != "":
        d=IEEE802154(data=data)
        fdesc.write(d)
        fdesc.flush()

