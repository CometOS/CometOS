BEGIN {
	numColumns = 0;
}
NR == 1{
	for (i = 1; i <= NF; i++) {
		if (index($i, word) > 0) {
			columns[i] = 1;
			numColumns++;
		} else {
			columns[i] = 0;
		}
		printf("%s\t", $i);
	}
	printf("%s\n", word);
}
NR > 1{
	sum = 0;
	for (i = 1; i <= NF; i++) {
		if (columns[i] == 1) {
			sum += $i;
		} 
		printf("%s\t", $i);
	}
	printf("%f\n", sum);
	
}
END {
}
