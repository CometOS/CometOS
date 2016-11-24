#!/usr/bin/env python
import re
import sys

nodes = {}

with open(sys.argv[1]) as log:
    for l in log:
        m = re.search('!(0x.*)!(0x.*)!(.)!(.)!(.*)',l)
        if m:
            sender = m.group(1)
            receiver = m.group(2)
            direction = m.group(3)
            dummy = (m.group(4) == "d")
            seq = m.group(5)
            print sender, receiver, direction, dummy, seq
            print l

            if not sender in nodes:
                nodes[sender] = {}
