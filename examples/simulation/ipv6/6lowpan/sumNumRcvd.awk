BEGIN {
	ini = True
}
FNR == 1 {
	if (!ini) { 
		print "sum="sum
	}
	sum = 0
}

/iterationvars2/ {
	print $0
}

/tg.*Num/ {
	sum+=$4
	print $4
}

END {
	print "sum="sum
}