source ../../statistics/basic.sh

RAW=./raw.log

rm -rf $RAW

for f in $(find $RESULT_DIR -name "*.sca"); do
	seed=$(extractParameter $f "attr seedset" 3)
	payload=$(extractParameter $f "attr payload" 3)
	grep "receiveCounter" $f | tr "[]." " " | cut -d " " -f3,4,8 | sed s/^/"payload $payload seed $seed "/ >> $RAW
	
done


for f in $(find $RESULT_DIR -name "*General-10-*.sca"); do
	seed=$(extractParameter $f "attr seedset" 3)
	grep "nbFramesWithoutInterference " $f | tr "[]." " " | cut -d " " -f3,4,9 | sed s/^/"payload -1 seed $seed "/ >> $RAW
done


