package cometos.examples.simulation.mac.range;

import cometos.examples.simulation.base.Node154;
import cometos.src.communication.traffic.PeriodicTraffic;
import cometos.src.communication.linkquality.LinkQualityAverager;


module Node extends Node154
{
    submodules:
       traffic: PeriodicTraffic {
            @display("p=96,31");
        }
        lqi: LinkQualityAverager {
            @display("p=96,98");
        }
    connections:
       
        lqi.gateReqOut --> nic.gateReqIn;
        nic.gateIndOut --> lqi.gateIndIn;
        lqi.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> lqi.gateReqIn;
}

