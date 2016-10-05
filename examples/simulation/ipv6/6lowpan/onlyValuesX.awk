BEGIN {
	column = 0;
}
NR == 1 {
	t = 0;
	for (i = 1; i <= NF; i++) {
		if  ($i == columnName) {
			column = i;
		} else {
			if (t == 1) {
				printf("\t");
			} else {
				t = 1;
			}
			str = $i;
			gsub("#", "", str);				
			gsub("\"", "", str);				
			printf("%s", str);
		}
	}
	printf("\n");
}
NR > 1 {
	t = 0;
	if ($column == value) {
		for (i = 1; i <= NF; i++) {
			if  (i != column) {
				if (t == 1) {
					printf("\t");
				} else {
					t = 1;
				}
				printf("%s", $i);
			}
		}
		printf("\n");
	}
}
END {
}
