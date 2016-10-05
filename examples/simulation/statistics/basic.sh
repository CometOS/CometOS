function extractParameter {
	if [ $#  -ne 3 ]; then 
		echo "ERROR in extractParameter";
		return; 
	fi
	grep "$2" $1 | cut -d " " -f $3               
} 

function extractString {
	if [ $#  -ne 2 ]; then 
		echo "ERROR in extractString";
		return; 
	fi
	echo $1 | cut -d " " -f $2               
} 
