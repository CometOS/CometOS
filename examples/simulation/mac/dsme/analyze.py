import re
import sys

#scalar Network.node[0].traffic 	received_001 	10
#scalar Network.node[1].traffic 	sent 	10

received = {}
sent = {}

for line in open("results/"+sys.argv[1]+"-0.sca"):
    m = re.search("received_unique_([0-9]+)\s+([0-9]+)", line)
    if m:
         received[int(m.group(1))] = int(m.group(2))
    else:
        m = re.search("node\[([0-9]+)\]\..*sent\s+([0-9]+)", line)
        if m:
            sent[int(m.group(1))] = int(m.group(2))

for k, v in sent.items():
    rel = 0
    try:
        rel = received[k]/(v*1.0)
    except:
        rel = 0

    print "%i: %f"%(k,rel)
    
