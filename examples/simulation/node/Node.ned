package cometos.examples.simulation.node;

import cometos.examples.simulation.base.Node154Flat;

import cometos.src.communication.association.AssociationService;
import cometos.src.communication.routing.SimpleRouting;
import cometos.src.communication.reliability.SimpleReliabilityLayer;
import cometos.src.communication.systemmonitor.SystemMonitor;
import cometos.src.communication.tcp.TcpComm;
import cometos.src.communication.base.Dispatcher;
import cometos.src.communication.base.RemoteAccess;
import cometos.src.communication.ieee802154.mac.NetworkInterfaceSwitch;
import cometos.src.communication.otap.Otap;


module Node extends Node154Flat
{
    submodules:

        tcp: TcpComm {
            @display("p=120,307;i=block/socket");
        }
        nwkDisp: Dispatcher {
            @display("p=175,169");
        }
        macDisp: Dispatcher {
            @display("p=175,237");
        }
        sys: SystemMonitor {
            @display("p=251,175");
        }
        remoteAccess: RemoteAccess {
            @display("p=175,23");
        }
        nis: NetworkInterfaceSwitch {
            @display("p=86,237");
        }
        otap: Otap {
            @display("p=86,93");
        }
        reliability: SimpleReliabilityLayer {
            @display("p=175,93");
        }
        routing: SimpleRouting {
            @display("p=86,169");
        }
        as: AssociationService {
            @display("p=251,93");
        }
    connections allowunconnected:

        mac.gateIndOut --> nis.cniIndIn;
        nis.cniReqOut --> mac.gateReqIn;

        nis.gateIndOut --> macDisp.gateIndIn;
        macDisp.gateReqOut --> nis.gateReqIn;

        nis.nniReqOut --> tcp.gateReqIn;
        tcp.gateIndOut --> nis.nniIndIn;

        nwkDisp.gateIndOut[1] --> otap.gateIndIn;
        otap.gateReqOut --> nwkDisp.gateReqIn[1];
        routing.gateReqOut --> macDisp.gateReqIn[1];
        macDisp.gateIndOut[1] --> routing.gateIndIn;
        routing.gateIndOut --> nwkDisp.gateIndIn;
        nwkDisp.gateReqOut --> routing.gateReqIn;

        nwkDisp.gateIndOut[2] --> reliability.gateIndIn;
        reliability.gateReqOut --> nwkDisp.gateReqIn[2];
        reliability.gateIndOut --> remoteAccess.gateIndIn;
        remoteAccess.gateReqOut --> reliability.gateReqIn;
        nwkDisp.gateIndOut[4] --> as.gateIndIn;
        as.gateReqOut --> nwkDisp.gateReqIn[4];
}

