package cometos.examples.simulation.mac.dsme;

import cometos.examples.simulation.base.Node154Flat;
import cometos.examples.common.mac.TrafficExample;

module Node extends Node154Flat
{
    submodules:
        traffic: TrafficExample {
            @display("p=156,36");
        }
    connections:
        mac.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> mac.gateReqIn;

        mac.gateSnoopIndOut --> traffic.gateSnoopIn;
}

