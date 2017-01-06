package cometos.examples.simulation.mac.dsme;

import cometos.examples.simulation.base.Node154Flat;
import cometos.src.communication.neighborhood.TCPWY;
import cometos.examples.common.traffic.TrafficEvaluation;
import cometos.src.communication.routing.LocationBasedRouting;
import cometos.src.communication.base.Dispatcher;

module Node extends Node154Flat
{
    submodules:
        traffic: TrafficEvaluation {
            @display("p=156,36");
        }
        
       // routing: LocationBasedRouting {
       // }
        
        neighbourhood: TCPWY {
            @display("p=174,36");
        }
        
        macDisp: Dispatcher {
            @display("p=175,237");
        }
        
    connections allowunconnected:
        mac.gateIndOut --> macDisp.gateIndIn;
        macDisp.gateReqOut --> mac.gateReqIn;
        
        // Use the neighbourhood management on a separate
        // branch of the layer tree so it does not use
        // piggybacking with strange side effects and instead
        // uses broadcast beacons only.
        neighbourhood.gateReqOut --> macDisp.gateReqIn[1];
        macDisp.gateIndOut[1] --> neighbourhood.gateIndIn;
        
        traffic.gateReqOut --> macDisp.gateReqIn[2];
        macDisp.gateIndOut[2] --> traffic.gateIndIn;
        
//        routing.gateIndOut --> traffic.gateIndIn;
//        traffic.gateReqOut --> routing.gateReqIn;
}

