#!/bin/bash

locationOfScript=$(dirname "$(readlink -e "$0")")
CORES=`grep -c ^processor /proc/cpuinfo`
NETWORKS="RealSim LongY N1-10 N8-2"
WORKSPACE="${locationOfScript%/cometos_v6/*}"
SIMRESULTS_DIR="${locationOfScript%%/workspace/*}/simresults"
CMODE="MODE=release"

## default to 105 runs 
RC_OFFSET=""

function helpmsg {
	echo "Usage $0 -r <DelayMode/Retrycontrol Offset> [-n <networks> -o <outputDir> -w <workspaceDir> -m <Assembly/Direct> -c <cores> -a <modeAdds>]"
}

while getopts "n:w:o:c:h:r:?" OPT; do
	case "$OPT" in
		n)	NETWORKS=$OPTARG
			;;
		w)	WORKSPACE=$OPTARG
			;;
		o)	SIMRESULTS_DIR=$OPTARG
			;;
		c)	CORES=$OPTARG
			;;
		r)  RUNS=$OPTARG
		    ;;
		h|?)
			helpmsg
			exit 0
			;;
	esac
done


echo "Starting with NETWORKS=$NETWORKS, CORES=$CORES, WORKSPACE=$WORKSPACE, SIMRESULTS_DIR=$SIMRESULTS_DIR, RUNS=0..$RUNS"

COMETOS_v6_PATH=$WORKSPACE/cometos_v6
COMETOS_PATH=$WORKSPACE/cometos
COMETOS_NED_FOLDERS=$COMETOS_v6_PATH/modules:$COMETOS_PATH/MiXiM/base:$COMETOS_PATH/MiXiM/modules:$COMETOS_PATH/src:$COMETOS_PATH/sim

cd $WORKSPACE/cometos
make $CMODE clean > /dev/null
if [ "$CORES" -gt "8" ]; then
	CCORES=8
else
	CCORES=$CORES
fi
echo "compiling cometos using $CCORES cores"
make $CMODE -j$CCORES > /dev/null

echo "Compiling cometos_v6"

cd $COMETOS_v6_PATH/sim/6lowpan
cd $COMETOS_v6_PATH
make $CMODE -j$CORES 1> /dev/null


mkdir -p $SIMRESULTS_DIR/${mode}

# run simulation
rm $COMETOS_v6_PATH/sim/6lowpan/results/*
for network in $NETWORKS; do
	(
	echo "Simulating $network in mode $mode"
	directive="opp_runall -j$CORES LD_LIBRARY_PATH=$COMETOS_PATH:$LD_LIBRARY_PATH $COMETOS_v6_PATH/cometos_v6 -u Cmdenv -n $COMETOS_NED_FOLDERS -l $COMETOS_PATH/cometos -c ${network} -r $RUNS"
	echo "$directive"
	$directive #> /dev/null
	) &
	
	wait
	if [[ -d results ]]; then
		cd results
		rm -rf "$SIMRESULTS_DIR/$network*.*"

		wait
		for i in *; do
			mv "$i" "$SIMRESULTS_DIR/$i" &
		done
		wait
		cd $WORKSPACE/cometos_v6/sim/6lowpan
	else
		wait
		echo "Failed running simulation, exiting..."
		exit -1
	fi
done
		

