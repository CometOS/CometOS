package cometos.examples.simulation.mac.timings;

import cometos.src.communication.ieee802154.mac.Ieee154Phy;
import cometos.src.communication.ieee802154.mac.SimpleMac;
import cometos.src.communication.traffic.PeriodicTraffic;

import org.mixim.base.modules.*;

module Node
{
    parameters:
        int id = default(0);
        string mobType; //type of the mobility module
        @display("bgb=268,302,white");
    gates:
        input radioIn; // to receive AirFrames
    submodules:
        mobility: <mobType> like IBaseMobility {
            parameters:
                y = 50;
                z = 0;
                x = id;
                @display("p=213,253;i=block/cogwheel");
        }

        utility: BaseUtility {
            parameters:
                @display("p=213,173;b=24,24,rect,black;i=block/blackboard");
        }
        nic: Ieee154Phy {
            @display("p=96,242");
        }
        traffic: PeriodicTraffic {
            @display("p=96,31");
        }

        mac: SimpleMac {
            @display("p=96,167");
        }
    connections:
        radioIn --> nic.radioIn;
        mac.lowerControlOut --> nic.upperControlIn;
        nic.upperControlOut --> mac.lowerControlIn;
        nic.upperGateOut --> mac.lowerGateIn;
        mac.lowerGateOut --> nic.upperGateIn;

        traffic.gateReqOut --> mac.gateReqIn;
        mac.gateIndOut --> traffic.gateIndIn;
}

