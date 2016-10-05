#!/bin/bash

CORES=$1
mode=$2
WORKSPACE=$3
CMODE=$4

cd $WORKSPACE/cometos_v6

FLAGS=""

if [[ "$mode" == *Direct* ]]; then
	FLAGS="$FLAGS -DLOWPAN_ENABLE_DIRECT_FORWARDING=1"
fi

echo "Compile with FLAGS=$FLAGS"

make $CMODE clean > /dev/null
make $CMODE -j$CORES CFLAGS="$FLAGS" CPPFLAGS="$FLAGS" 1> /dev/null

