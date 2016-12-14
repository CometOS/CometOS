#!/usr/bin/env python
from scapy.all import *
import time
import subprocess
import fileinput

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
    print line,
    data = ""
    try:
        data = ''.join(map(lambda x: chr(int(x,16)), filter(lambda x: "0x" in x, line.split(" "))))
    except:
        pass # could not decode

    if data != "":
        d=IEEE802154(data=data)
        fdesc.write(d)
        fdesc.flush()

