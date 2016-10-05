Simulation Description
-----------------------
author: Stefan Unterschuetz
date: 26.09.2012

Tests propagation model, by sending 1000 packets of different payload size
 (exclusively of MAC header) to nodes of distances 1,2,3,..150 m.
 
Processing chain calculates mean and standard deviation for each distance and
packet size (9 seeds are used). Furthermore, the maximum propagation range
is logged (as payload -1).


