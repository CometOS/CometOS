# library which contains functions to evaluate string expressions
@include "lib_eval.awk";
@include "lib_omnetpp.awk";

BEGIN {
    if (resultsFile == "") {
        print "Exiting, no resultsFile specified (-v resultsFile=<filename>)"
        exit -1
    }
	numResultValues = 0;
	if (printHeader == 0) {
		numTotalValues = 0;
		while (getline < headerFile > 0) {
			oldLabels[numTotalValues] = $1;
			numTotalValues++;
		}
	}
}

/attr iterationvars2/ {
    numParameters = parseIterationVarsLine($0, nArray, vArray);
    for (j = 0; j < numParameters; j++) {
        x[j] = vArray[j+1]
        xnames[j] = nArray[j+1]
    }
}

$1 ~ /scalar/ {
	split($2, a, ".");
	if (a[1] == "Network") {
	
	    # record names of format: scalar Network.node[<node>].<module> <stat> <value>
	    # <stat> might consist of a single name or a quoted name containing whitespace
	    # this whitespace is replaced by an underscore
		t = (a[2] "." a[3] "." $3);
		for (j = 4; j < NF; j++) {
			t = (t) "_" ($j);
		}
		
		# remove quotation marks
		s = split(t, a, "\"");
		names[numResultValues] = "";
		for (j = 1; j <= s; j++) {
			names[numResultValues] = (names[numResultValues]) (a[j]);
		}
		
		# store actual numeric value
		values[numResultValues] = $NF;
		numResultValues++;
	}
}


END {
	if (printHeader == 1) { 
		system("rm " headerFile);
		
		# print iteration variable names
		for (j = 0; j < numParameters; j++) {
			printf("%s\t", xnames[j]) >> resultsFile;
			printf("%s\n", xnames[j]) > headerFile;
		}
		
		# print result value names
		for (j = 0; j < numResultValues; j++) {
			printf("%s\t", names[j]) >> resultsFile;
			printf("%s\n", names[j]) > headerFile;
		}
		printf("\n") >> resultsFile;
		
		# print iteration variable values
		for (j = 0; j < numParameters; j++) {
			printf("%d\t", x[j]) >> resultsFile;
		}
		
		# print result values
		for (j = 0; j < numResultValues; j++) {
			printf("%.2f\t", values[j]) >> resultsFile;
		}
	} else {
		for (j = 0; j < numParameters; j++) {
			for (l = 0; l < numParameters; l++) {
				if (oldLabels[j] == xnames[l]) {
					printf("%d\t", x[l]) >> resultsFile;					
					p = 1;
				}
			}
		}
		for (j = numParameters; j < numTotalValues; j++) {
			p = 0;
			for (l = 0; l < numResultValues; l++) {
				if (oldLabels[j] == names[l]) {
					printf("%.2f\t", values[l]) >> resultsFile;					
					p = 1;
				}
			}
			if (p == 0) {
			    printf("-1\t") >> resultsFile;
			}
		}
		
	}
 	
	printf("\n") >> resultsFile;
}
