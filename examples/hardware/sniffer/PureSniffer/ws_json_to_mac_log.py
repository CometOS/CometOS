#!/usr/bin/python
import csv
import sys

pendingAcks = {}

with open(sys.argv[1], 'rb') as csvfile:
    reader = csv.reader(csvfile, delimiter=',', quotechar='"')
    headers = reader.next()

    for row in reader:
        d = dict(zip(headers,row))
        print d
        time = float(d['Time'])
        sender = d['Source']
        ptype = "A"
        duration = int(d['Length'])*2 # two symbols per byte
        if d['Info'] != 'Ack':

        print "[INFO]\t%f\t%s: 0|0|0|%s|%i"%(time,sender,ptype,duration)
