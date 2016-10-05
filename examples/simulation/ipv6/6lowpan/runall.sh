#!/bin/bash

locationOfScript=$(dirname "$(readlink -e "$0")")
CORES=`grep -c ^processor /proc/cpuinfo`
NETWORKS="RealSim LongY N1-10 N8-2"
WORKSPACE="${locationOfScript%/cometos_v6/*}"
SIMRESULTS_DIR="${locationOfScript%%/workspace/*}/simresults"
MODES="Assembly Direct"
MODEADDS="- -DO -RR -ARR -PRC -DO-PRC -RR-PRC -ARR-PRC"
CMODE="MODE=release"

## default to 105 runs 
RC_OFFSET=""

function helpmsg {
	echo "Usage $0 -r <DelayMode/Retrycontrol Offset> [-n <networks> -o <outputDir> -w <workspaceDir> -m <Assembly/Direct> -c <cores> -a <modeAdds>]"
}

while getopts "n:w:o:c:m:a:h:r:?" OPT; do
	case "$OPT" in
		n)	NETWORKS=$OPTARG
			;;
		w)	WORKSPACE=$OPTARG
			;;
		o)	SIMRESULTS_DIR=$OPTARG
			;;
		c)	CORES=$OPTARG
			;;
		m)	MODES=$OPTARG
			;;
		a)	MODEADDS=$OPTARG
			;;
		r)  RC_OFFSET=$OPTARG
			;;
		h|?)
			helpmsg
			exit 0
			;;
	esac
done

if [[ RC_OFFSET == "" ]]; then
	echo "Number of runs for each DelayMode/RetryControl configuration has to be given"
	helpmsg
	exit -1
fi

echo "Starting with NETWORKS="$NETWORKS", CORES=$CORES, MODES="$MODES", MODEADDS="$MODEADDS", WORKSPACE=$WORKSPACE, SIMRESULTS_DIR=$SIMRESULTS_DIR"

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

for modec in $MODES
do
	echo "Compiling for Mode $modec"

	cd $COMETOS_v6_PATH/sim/6lowpan
	./compile.sh $CCORES $modec $WORKSPACE $CMODE


	DM_OFFSET=$((5 * $RC_OFFSET))

	for modeAdd in $MODEADDS; do

        [ "$modec" == "Assembly" ] && [[ "$modeAdd" == *-RR* ]] && continue
        [ "$modec" == "Assembly" ] && [[ "$modeAdd" == *-DO* ]] && continue
        [ "$modec" == "Assembly" ] && [[ "$modeAdd" == *-ARR* ]] && continue


		RUN_L=0
		if [[ "$modeAdd" == *-SRC* ]]; then
			RUN_L=$RC_OFFSET
		fi

		if [[ "$modeAdd" == *-PRC* ]]; then
			RUN_L=$((2 * $RC_OFFSET))
		fi

		if [[ "$modeAdd" == *-ASPRC* ]]; then
			RUN_L=$((3 * $RC_OFFSET))
		fi

		if [[ "$modeAdd" == *-MSPRC* ]]; then
			RUN_L=$((4 * $RC_OFFSET))
		fi

		if [[ "$modeAdd" == *-RR* ]]; then
			RUN_L=$(($DM_OFFSET + RUN_L))
		fi
		if [[ "$modeAdd" == *-DO* ]]; then
			RUN_L=$((2 * DM_OFFSET + RUN_L))
		fi
		if [[ "$modeAdd" == *-ARR* ]]; then
			RUN_L=$((3 * $DM_OFFSET + RUN_L))
		fi

		RUN_H=$((RUN_L + $RC_OFFSET - 1))
		RUNS="${RUN_L}..${RUN_H}"


		if [[ "$modeAdd" != "-" ]]; then
			mode="$modec$modeAdd"
		else
			mode="$modec"			
		fi
	
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
				rm -rf "$SIMRESULTS_DIR/${mode}/$network*.*"
		
				wait
				for i in *; do
					mv "$i" "$SIMRESULTS_DIR/${mode}/$i" &
				done
				wait
				cd $WORKSPACE/cometos_v6/sim/6lowpan
			else
				wait
				echo "Failed running simulation, exiting..."
				exit -1
			fi
		done
	done

done
		

