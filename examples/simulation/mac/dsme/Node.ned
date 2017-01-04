package cometos.examples.simulation.mac.dsme;

import cometos.examples.simulation.base.Node154Flat;
import cometos.src.communication.neighborhood.TCPWY;
import cometos.examples.common.traffic.TrafficEvaluation;
import cometos.src.communication.routing.LocationBasedRouting;

module Node extends Node154Flat
{
    submodules:
        traffic: TrafficEvaluation {
            @display("p=156,36");
        }
        
        //routing: LocationBasedRouting {
        //}
        
       // neighbourhood: TCPWY {
       //     @display("p=174,36");
       // }
        
    connections allowunconnected:
        mac.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> mac.gateReqIn;
        
       // neighbourhood.gateIndOut --> routing.gateIndIn;
      //  routing.gateReqOut --> neighbourhood.gateReqIn;
        
      //  routing.gateIndOut --> traffic.gateIndIn;
      //  traffic.gateReqOut --> routing.gateReqIn;
}

