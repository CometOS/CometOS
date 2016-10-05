source ../../statistics/basic.sh

RAW=./raw.log

rm -rf $RAW

for f in $(find $RESULT_DIR -name "*.sca"); do
	dist=$(echo $f | cut -d "-" -f2)
	seed=$(extractParameter $f "attr seedset" 3)
	grep "receiveCounter" $f | tr "[]." " " | cut -d " " -f3,4,8 | sed s/^/"dist $dist seed $seed "/ >> $RAW
	
done



