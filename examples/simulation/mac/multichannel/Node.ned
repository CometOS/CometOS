package cometos.examples.simulation.mac.multichannel;

import cometos.src.communication.ieee802154.mac.Ieee154Phy;
import cometos.src.communication.ieee802154.mac.SimpleCsmaMac;
import cometos.src.communication.linkquality.LinkQualityAverager;
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

        simpleCsmaMac: SimpleCsmaMac {
            @display("p=96,167");
        }
        linkQuality: LinkQualityAverager {
            @display("p=96,100");
        }
    connections:
        radioIn --> nic.radioIn;
        simpleCsmaMac.lowerControlOut --> nic.upperControlIn;
        nic.upperControlOut --> simpleCsmaMac.lowerControlIn;
        nic.upperGateOut --> simpleCsmaMac.lowerGateIn;
        simpleCsmaMac.lowerGateOut --> nic.upperGateIn;

        simpleCsmaMac.gateIndOut --> linkQuality.gateIndIn;
        linkQuality.gateReqOut --> simpleCsmaMac.gateReqIn;
        linkQuality.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> linkQuality.gateReqIn;
}

