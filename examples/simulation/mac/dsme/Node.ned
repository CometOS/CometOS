package cometos.examples.simulation.mac.dsme;

import cometos.examples.simulation.base.Node154Flat;
import cometos.examples.common.traffic.TrafficEvaluation;

module Node extends Node154Flat
{
    submodules:
        traffic: TrafficEvaluation {
            @display("p=156,36");
        }
        
    connections allowunconnected:
        mac.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> mac.gateReqIn;
}

