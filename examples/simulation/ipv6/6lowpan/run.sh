#!/bin/bash
COMETOS_DIR=../../../cometos
NED_DIRS=$COMETOS_DIR/MiXiM/base:$COMETOS_DIR/MiXiM/modules:$COMETOS_DIR/src:$COMETOS_DIR/sim:../../modules

valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=valgrind.lg ../../Cometos_v6_lffr -n $NED_DIRS -c TestN1-10 -r 0  -u Cmdenv omnetpp.ini

