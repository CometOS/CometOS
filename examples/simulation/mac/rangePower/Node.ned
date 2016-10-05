package cometos.examples.simulation.mac.rangePower;

import cometos.src.communication.power.PowerAdaptation;
import cometos.examples.simulation.base.Node154;
import cometos.src.communication.traffic.PeriodicTraffic;
import cometos.src.communication.linkquality.LinkQualityAverager;


module Node extends Node154
{
    submodules:
        traffic: PeriodicTraffic {
            @display("p=125,23");
        }
        lqi: LinkQualityAverager {
            @display("p=86,185");
        }
        powerAdaptation: PowerAdaptation {
            @display("p=86,111");
        }
    connections:

        lqi.gateReqOut --> nic.gateReqIn;
        nic.gateIndOut --> lqi.gateIndIn;
        traffic.gateReqOut --> powerAdaptation.gateReqIn;
        powerAdaptation.gateIndOut --> traffic.gateIndIn;
        powerAdaptation.gateReqOut --> lqi.gateReqIn;
        lqi.gateIndOut --> powerAdaptation.gateIndIn;
}

