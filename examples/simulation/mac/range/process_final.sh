AWK_STATS=../../statistics/basic.awk


RAW=./raw.log
FINAL=./range.dat

echo "# distance pay10 err10 pay20 err20 pay50 err50 pay100 err100 pay-1 err-1" > $FINAL
for node in {20..100}; do
#for node in 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80 82 84 86 88 90; do
	result="$node"
	for payload in 10 20 50 100 -1; do
		result=$result" "$(cat $RAW | grep "payload $payload " | grep "node $node " | cut -d " " -f 7 | awk -f $AWK_STATS | cut -d " " -f 4,8)
	done
	echo $result >> $FINAL
done



