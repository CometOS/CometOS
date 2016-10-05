# file expects the same file twice, first iteration calculates an etx if non 
# is present, second iteration writes original files + etx to new file
# additionally calculates a "weight" which is to be used for a Dijkstra 
# algorithm to determine routing paths 
BEGIN {
    ## rssiTh before 2015-11 was -85.0!!!
	rssiTh = -80.0
	rssiPenalty = 0.1
	rssiVarPenalty = 0.002
} 

# skip header, first iteration
NR != 1 && NR == FNR {
    if (!max[$2]) {
        max[$2]=int($5)
    } else {
        if (max[$2] < $5) {
            max[$2] = int($5)
        }
    }
}

NR ==1 {
	if (etxPresent) {
		print $0, "weight"
	} else {
    	print $0, "etx", "weight"
    }
}
FNR != 1 && NR != FNR {
	if (etxPresent == "") {
		etx = 1/((int($5)*1.0)/max[$2])
	} else {
		etx = $6
	}	
	
	# weight algorithm (hardware specific):
	# add rssiPenalty per RSSI avg smaller than -75 dBm (pre-2015-11 -85 dBm) to ext value
	# add rssiVarPenalty per RSSI variance to etx value 
	weight = etx
	if ($3 < rssiTh) {
		weight += rssiPenalty * (rssiTh - $3)
	}
	weight += rssiVarPenalty * $4

	# print dst src rssiAvg rssiVar numRcvd etx weight
	print $1,$2,$3,$4,$5,etx,weight
}

END {
	if (bsNode != "" && bsId != "") {
		serialLink = "-50 0 1 1"
		printf("%d %d %s\n", strtonum(bsId), strtonum(bsNode), serialLink)
		printf("%d %d %s\n", strtonum(bsNode), strtonum(bsId), serialLink)
	}
}

