{
# prints line that matches (key,value) pair
# usage: pass key and value pair via  -v key=$key -v value=$value 
	for(i=1;i<=NF;i++){
		if ($i==key && $(i+1)==value) {
			print $0; 
			break;
		}
   }
}
