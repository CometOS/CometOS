#!/usr/bin/env python
import re
import sys

with open(sys.argv[1]) as log:
    for l in log:
        m = re.search('!(0x.*)!(.)!(.)!(.*)',l)
        if m:
            myid = m.group(1)
            direction = m.group(2)
            dummy = (m.group(3) == "d")
            seq = m.group(4)
            print myid, direction, dummy, seq
            print l
