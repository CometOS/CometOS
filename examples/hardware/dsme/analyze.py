#!/usr/bin/env python
import re
import sys

nodes = {}
seqs = {}

with open(sys.argv[1]) as log:
    for l in log:
        m = re.search('!(0x.*)!(0x.*)!(.)!(.)!(.*)',l)
        if m:
            sender = m.group(1)
            receiver = m.group(2)
            direction = m.group(3)
            dummy = (m.group(4) == "d")
            seq = m.group(5)
            #print sender, receiver, direction, dummy, seq

            if dummy:
                continue

            if not sender in nodes:
                nodes[sender] = {}
                nodes[sender]["sent"] = 0
                nodes[sender]["duplicates"] = 0
                nodes[sender]["received"] = 0
                seqs[sender] = set()

            if direction == "T":
                nodes[sender]["sent"] += 1
            elif direction == "R":
                if seq in seqs[sender]:
                    nodes[sender]["duplicates"] += 1
                else:
                    nodes[sender]["received"] += 1
                seqs[sender].add(seq);
                

    for n in nodes:
        nodes[n]["PRR"] = nodes[n]["received"]/float(nodes[n]["sent"])

    print nodes

            
