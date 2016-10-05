BEGIN{
n=0;
}
{
	if(min=="")  {
		min=max=$1
	}
	
	if($1>max) {max=$1;} 
	if($1< min) {min=$1;}

    n++;
    sum1 += $1;
    sum2 += $1*$1;
}
END {
	if (n<1) {
  		mean=0
    } else {
      	mean=sum1/n;
    }
    
    if (n<2) {
  	  stdev=0
    } else {
	    temp=(sum2-n*mean*mean)/(n-1);
	    if (temp<0) {
	    	stdev=0
	    } else {
	        stdev=sqrt((sum2-n*mean*mean)/(n-1))
	    } 
    }
    if (n<1) {
		stderr=0
    } else {
    	stderr=stdev/sqrt(n)
    }
    print "n "n" mean "mean" stdev "stdev" stderr "stderr" min "min" max "max
    
    }
