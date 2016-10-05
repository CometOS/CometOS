package cometos.examples.simulation.neighborhood;

import cometos.src.communication.ieee802154.mac.IMacAbstractionLayer;
import cometos.src.communication.neighborhood.TCPWY;
import cometos.examples.simulation.base.Node154Flat;


module Node extends Node154Flat
{
    submodules:
        tcpwy: TCPWY {
            @display("p=174,36");
        }

    connections allowunconnected:
        mac.gateIndOut --> tcpwy.gateIndIn;
        tcpwy.gateReqOut --> mac.gateReqIn;


}


