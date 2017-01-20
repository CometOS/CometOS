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
    if False:
        if "m3" in line:
            # Probably IotLab log
            # TODO parse time
            #m = re.search(';(.*)',line)
            m = re.search(";.*;(0x.*)",line)
            if m:
                line = m.group(1)
            else:
                continue
                #print line

    #m = re.search("([0-9.]*)\;.*\;PKT(.*)",line)
    m = re.search("PKT(.*):(.*)",line)
    if m:
        time = int(m.group(1),16)*(16/(1000.0*1000.0))
        line = m.group(2)
        
        print time,line

        data = []
        for i in range(0,len(line)-1,2):
            data.append("0x"+line[i:i+2])

        data = ''.join(map(lambda x: chr(int(x,16)), data))

        #except:
        #    pass # could not decode

        if data != "":
            d=IEEE802154(data=data)
            d.time = float(time)

            fdesc.write(d)
            fdesc.flush()
