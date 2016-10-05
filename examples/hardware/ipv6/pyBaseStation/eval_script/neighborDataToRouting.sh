#!/bin/bash

# Creates routing output files which can be used by the StaticConnectionManager
# for MiXiM and the cometos_v6 pyBasestation as well as linkstats for the 
# EmpiricPhy layer for MiXiM. Additionally, .dot visualization of those 
# topologies are created and transformed into a pdf. Also, the shortest paths
# file containing the output of the Dijkstra algorithm is created.
# Inputs are:
#    - a directory containing topology files (.raw) with the following format: 
#           <dst> <src> <rssiAvg> <rssiVar> <numRcvd> <etx>
#           platform/pyBaseStation/modules/Topology.py produces such a format 
#    - address of the basestation node
#    - address of the node attached to the basestation (between those two, 
#           a link with perfect quality will be created for the simulation)
#    - classpath to bin of Unterschuetz java graph library and apache commons-io 2.4
#      and graph visualization main program
#            https://svn.ti5.tu-harburg.de/telematics/trunk/staff/unterschuetz/java
#			 https://svn.ti5.tu-harburg.de/projects/trunk/wsn/cometos_util/TopologyToRouting
#            
# TODO: generated routing files do not look for the longest non-overlapping prefix 
# 

# default values
indir="."
bsId=""
bsNode=""

maindir=""
graphtoolsdir=""
graphMain="de.tuhh.ti5.cometos.util.GraphVisualization"

function helpmsg {
	echo -e "Usage $0 \n-d <inputdir with .raw topology files> \n-b <BS node id> \n-a <BS-attached node id> \n-g <basedir of unterschuetz graph tools> \n-m <basedir of main class> \n-v <vertices file (optional)\n-f <fileFilter>"
}

fileFilter=""

while getopts "d:b:a:p:g:m:f:v:h?" OPT; do
	case "$OPT" in
		d)	indir=$OPTARG
			;;
		b)	bsId=$OPTARG
			;;
		a)	bsNode=$OPTARG
			;;
		g)  graphtoolsdir="$OPTARG"
			;;
		m)  maindir="$OPTARG"
			;;
		f)  fileFilter="$OPTARG"
		    ;;
		v)  verticesfile="$OPTARG"
			;;
		h|?)
			helpmsg
			exit 0
			;;
	esac
done

if [ "$maindir" == "" ] || [ "$graphtoolsdir" == "" ]; then
	helpmsg
	exit -1
fi


commonsdir="${maindir}/commons-io-2.4/"
classpath="${commonsdir}/commons-io-2.4.jar:${maindir}/bin:${graphtoolsdir}/bin"

outdir=${indir}/processed
graphCmd="java -cp ${classpath} $graphMain"

echo "Running on $indir with bsAddr=$bsId and bsAttachedNodeAddr=$bsNode; fileFilter=$fileFilter"

for file in `find  ${indir} -maxdepth 1 | grep .raw`; do
    echo $file
    if [[ "$file" =~ "$fileFilter" ]]; then
    	if ! [[ $file =~ "tmp" ]]; then
    		filename="${file%.*}"
    		basename="${filename##*/}"
    		target=${outdir}/${basename}
    		echo ""
    		echo ""		
    		mkdir -p $target
    		echo "#################################################"
    		echo "processing $file" 
    		echo "#################################################"
    		echo "Target dir is $target"
    		tmpFile="${file}_tmp"
    		if [[ "$bsNode" == "" ]]; then
    		  python toLinkStats.py -f ${file} -o "${target}/${basename}_links.xml"
    		else
    		  python toLinkStats.py -f ${file} -o "${target}/${basename}_links.xml" -n $bsNode -b $bsId
    		fi
    		header=$(head -1 $file)
    		if [[ $header =~ "etx" ]]; then
    			etxPresent=1
    		fi
    		awk -f addWeight.awk -v etxPresent=$etxPresent -v bsNode=$bsNode -v bsId=$bsId $file $file > $tmpFile
    		echo $graphCmd ${tmpFile} ${target} mapping.csv $bsId
    		$graphCmd ${tmpFile} ${target} mapping.csv $bsId
                    if [ ! -z "$verticesfile" ]; then
                          neato -Tpdf ${target}/${basename}.dot > ${target}/${basename}_neato.pdf
                          neato -Tpdf ${target}/${basename}_st.dot > ${target}/${basename}_st_neato.pdf
                    fi
    		rm $tmpFile
    		python shortestPathsToRouting.py -f ${target}/${basename}.sp -o "${target}/${basename}_routingHw.xml" -s "${target}/${basename}_routingSim.xml" -b $bsId
    	fi
    fi
		
done
