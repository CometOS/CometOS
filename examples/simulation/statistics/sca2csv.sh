#!/bin/bash

SIMRESULTS_DIR="results"

PRINT_HEADER=1
for i in $SIMRESULTS_DIR/*.sca; do
	#echo "Running getValues.awk on $i"
	awk -f getValues.awk \
				-v printHeader=$PRINT_HEADER -v headerFile=header.txt \
				-v resultsFile=$SIMRESULTS_DIR.csv \
				"$i" 
	PRINT_HEADER=0
done		

