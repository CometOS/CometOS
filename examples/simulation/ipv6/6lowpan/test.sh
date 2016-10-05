#!/bin/bash
# script expected to be run from sim/6lowpan dir

path_to_cometos_base=/home/geo/bin/omnetpp-4.4.1/samples/cometos
path_to_cometos_lffr_prj=/home/geo/bin/omnetpp-4.4.1/samples/Cometos_v6_lffr
path_to_sim_exe=$path_to_cometos_lffr_prj/Cometos_v6_lffr

ned_folders=$path_to_cometos_lffr_prj/modules:$path_to_cometos_base/MiXiM/base:$path_to_cometos_base/MiXiM/modules:$path_to_cometos_base/sim:$path_to_cometos_base/src

opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c DirectModeBE3 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c DirectRRModeBE3 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c DirectARRModeBE3 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c DirectModeBE5 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c DirectRRModeBE5 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c DirectARRModeBE5 -r 0..100 omnetpp.ini

opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c LFFRDirectModeBE3 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c LFFRDirectRRModeBE3 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c LFFRDirectARRModeBE3 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c LFFRDirectModeBE5 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c LFFRDirectRRModeBE5 -r 0..100 omnetpp.ini
opp_runall -j64 $path_to_sim_exe -n $ned_folders -u Cmdenv -c LFFRDirectARRModeBE5 -r 0..100 omnetpp.ini

