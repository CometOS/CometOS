#!/bin/bash
COMETOS_DIR=../../../cometos
NED_DIRS=$COMETOS_DIR/MiXiM/base:$COMETOS_DIR/MiXiM/modules:$COMETOS_DIR/src:$COMETOS_DIR/sim:../../modules

valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=valgrind.lg ../../cometos_v6 -n $NED_DIRS -c RealSim -r 0  -u Cmdenv -l../../../cometos/cometos omnetpp.ini 
