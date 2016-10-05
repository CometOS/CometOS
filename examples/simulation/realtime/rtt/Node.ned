package cometos.examples.simulation.realtime.rtt;

import cometos.src.communication.ieee802154.mac.Ieee154Phy;
import cometos.src.communication.ieee802154.mac.NetworkInterfaceSwitch;
import cometos.src.communication.ieee802154.mac.SimpleCsmaMac;
import cometos.src.communication.ieee802154.mac.TreeForwarder;
import cometos.src.omnetpp.TcpConnector;

import org.mixim.base.modules.*;

module Node
{
    parameters:
        int id = default(0);
        string mobType; //type of the mobility module
        @display("bgb=268,342,white");
    gates:
        input radioIn; // to receive AirFrames
    submodules:
        utility: BaseUtility {
            parameters:
                @display("p=207,215;b=24,24,rect,black;i=block/blackboard");
        }
        mobility: <mobType> like IBaseMobility {
            parameters:
                @display("p=207,297;i=block/cogwheel");
        }
        mac: SimpleCsmaMac {
            @display("p=116,215");
        }
        nic: Ieee154Phy {
            @display("p=116,297");
        }
        macProxy: NetworkInterfaceSwitch {
            @display("p=77,134");
        }
        tcp: TcpConnector {
            @display("p=38,215");
        }
        traffic: TreeForwarder {
            @display("p=77,34");
        }
    connections:
        nic.upperControlOut --> mac.lowerControlIn;
        nic.upperGateOut --> mac.lowerGateIn;
        mac.lowerControlOut --> nic.upperControlIn;
        mac.lowerGateOut --> nic.upperGateIn;

        radioIn --> nic.radioIn;
        macProxy.nniReqOut --> tcp.gateReqIn;
        macProxy.cniReqOut --> mac.gateReqIn;
        mac.gateIndOut --> macProxy.cniIndIn;
        tcp.gateIndOut --> macProxy.nniIndIn;
        traffic.gateReqOut --> macProxy.gateReqIn;
        macProxy.gateIndOut --> traffic.gateIndIn;
}

