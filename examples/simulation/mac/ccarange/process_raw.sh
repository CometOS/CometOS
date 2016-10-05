source ../../statistics/basic.sh

RAW=./raw.log

rm -rf $RAW

for f in $(find $RESULT_DIR -name "*.sca"); do
	dist=$(extractParameter $f "attr dist" 3)
	echo $dist" "$(grep "node\[0\]" $f | grep "sendingFailed" | cut -d " " -f 4) 
	
done



