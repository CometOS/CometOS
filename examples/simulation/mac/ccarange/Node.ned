package cometos.examples.simulation.mac.ccarange;


import cometos.examples.simulation.base.Node154;
import cometos.src.communication.traffic.PeriodicTraffic;



module Node extends Node154
{
    parameters:
        
   submodules:
       traffic: PeriodicTraffic {
            @display("p=96,31");
        }
    
    connections:
             
        nic.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> nic.gateReqIn;
}

