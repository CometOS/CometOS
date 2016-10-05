package cometos.examples.simulation.sniffer;

import cometos.src.communication.traffic.PeriodicTraffic;
import cometos.examples.common.sniffer.SnifferExample;
import cometos.examples.simulation.base.Node154;


module Node extends Node154
{
    @display("bgb=304,354");
    submodules:
        sniffer: SnifferExample {
            @display("p=47,177");
        }
        traffic: PeriodicTraffic {
            @display("p=47,55");
        }
    connections:
        nic.gateIndOut --> sniffer.gateIndIn;
        sniffer.gateReqOut --> nic.gateReqIn;

        sniffer.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> sniffer.gateReqIn;
}

