#!/usr/bin/python
import csv
import json
import sys

pendingAcks = {}
maxnum = 0
addr_to_num = {}

def addr(l,t):
    global maxnum, addr_to_num
    if "wpan.%s16"%t in l['wpan']:
        addr = "0x%04x"%int(l['wpan']["wpan.%s16"%t],0)
    elif "wpan.%s64"%t in l['wpan']:
        addr = l['wpan']["wpan.%s64"%t].split(":")
        addr = "0x%04x"%int("%s%s"%(addr[-2],addr[-1]),16)
    else:
        addr = "0x0"

    if addr not in addr_to_num:
        maxnum += 1
        addr_to_num[addr] = maxnum

    return addr
    

with open(sys.argv[1]) as data_file:    
    data = json.load(data_file)
    newdata = []

    for d in data:
        l = d['_source']['layers']
        time = float(l['frame']['frame.time_relative'])
        src = addr(l,"src")
        dst = addr(l,"dst")

        try:
            seq = l['wpan']['wpan.seq_no']
            ack_req = l['wpan']['wpan.fcf_tree']['wpan.ack_request']
            if ack_req:
                pendingAcks[seq] = dst
        except:
            seq = 0
            ack_req = 0
        #print pendingAcks
        #print l
        ftype = int(l['wpan']['wpan.fcf_tree']["wpan.frame_type"],0)
        print dst
        if ftype == 0:
            ptype = "BEACON"
        elif ftype == 1 and dst == '0xffff':
            ptype = "BROADCAST"
        elif ftype == 1 and dst != '0xffff':
            ptype = "DATA"
        elif ftype == 2:
            ptype = "ACK"
        else:
            ptype = "OTHER"
        duration = int(l['frame']['frame.len'])*2 # two symbols per byte
        duration += 2*4 # Preamble
        duration += 2*1 # SFD
        duration += 2*1 # PHY Header
        #if d['Info'] != 'Ack':
        newdata.append({'time':time,'src':src,'type':ptype,'duration':duration})

    for d in newdata:
        print "[INFO]\t%f\t%s: 0|0|0|%s|%i"%(d['time'],addr_to_num[d['src']],d['type'],d['duration'])

print addr_to_num
