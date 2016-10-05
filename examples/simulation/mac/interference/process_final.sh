AWK_STATS=../../statistics/basic.awk


RAW=./raw.log
FINAL=./interf.dat

echo "# interference n10 n20 n30 p40 p60 p80" > $FINAL
for node in {1..50}; do
#for node in 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70; do
	result="$node"
	for dist in n10 n20 n30 p40 p60 p80; do
		result=$result" "$(cat $RAW | grep "dist $dist " | grep "node $node " | cut -d " " -f 7 | awk -f $AWK_STATS | cut -d " " -f 4,8)
	done
	echo $result >> $FINAL
done



