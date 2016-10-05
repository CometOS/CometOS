package cometos.examples.simulation.mac.interference;

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
                x = default(id+50);
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

        simpleMac: SimpleMac {
            @display("p=96,167");
        }
    connections:
        radioIn --> nic.radioIn;
        simpleMac.lowerControlOut --> nic.upperControlIn;
        nic.upperControlOut --> simpleMac.lowerControlIn;
        nic.upperGateOut --> simpleMac.lowerGateIn;
        simpleMac.lowerGateOut --> nic.upperGateIn;

        traffic.gateReqOut --> simpleMac.gateReqIn;
        simpleMac.gateIndOut --> traffic.gateIndIn;
}

