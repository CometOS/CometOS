package cometos.examples.simulation.realtime.basic;

import cometos.src.communication.base.Dispatcher;
import cometos.src.communication.base.RemoteAccess;
import cometos.src.communication.ieee802154.mac.CommAssessment;
import cometos.src.communication.ieee802154.mac.CsmaMac;
import cometos.src.communication.ieee802154.mac.Ieee154Phy;
import cometos.src.communication.ieee802154.mac.NetworkInterfaceSwitch;
import cometos.src.communication.otap.Otap;
import cometos.src.communication.routing.Routing;
import cometos.src.communication.systemmonitor.SystemMonitor;
import cometos.src.communication.tcp.TcpComm;
import cometos.src.communication.traffic.PeriodicTraffic;

import org.mixim.base.modules.*;

module Node
{
    parameters:
        int id = default(0);
        string mobType; //type of the mobility module
        @display("bgb=315,422,white");
    gates:
        input radioIn; // to receive AirFrames
    submodules:
        utility: BaseUtility {
            parameters:
                @display("p=272,310;b=24,24,rect,black;i=block/blackboard");
        }
        mobility: <mobType> like IBaseMobility {
            parameters:
                @display("p=272,382;i=block/cogwheel");
        }
        mac: CsmaMac {
            @display("p=117,310");
        }
        nic: Ieee154Phy {
            @display("p=117,382");
        }
        tcp: TcpComm {
            @display("p=39,310;i=block/socket");
        }
        routing: Routing {
            @display("p=78,160");
        }
        nwkDisp: Dispatcher {
            @display("p=78,84");
        }
        macDisp: Dispatcher {
            @display("p=179,238");
        }
        sys: SystemMonitor {
            @display("p=272,238");
        }
        remoteAccess: RemoteAccess {
            @display("p=179,29");
        }
        t1: PeriodicTraffic {
            @display("p=179,176");
        }
        t2: PeriodicTraffic {
            @display("p=179,100");
        }
        ca: CommAssessment {
            @display("p=179,310");
        }
        nis: NetworkInterfaceSwitch {
            @display("p=78,238");
        }
        otap: Otap {
            @display("p=78,22");
        }
    connections:
        nic.upperControlOut --> mac.lowerControlIn;
        nic.upperGateOut --> mac.lowerGateIn;
        mac.lowerControlOut --> nic.upperControlIn;
        mac.lowerGateOut --> nic.upperGateIn;

        radioIn --> nic.radioIn;
        nwkDisp.gateReqOut --> routing.gateReqIn;
        routing.gateIndOut --> nwkDisp.gateIndIn;
        macDisp.gateIndOut[0] --> routing.gateIndIn;
        routing.gateReqOut --> macDisp.gateReqIn[0];
        remoteAccess.gateReqOut --> nwkDisp.gateReqIn[0];
        nwkDisp.gateIndOut[0] --> remoteAccess.gateIndIn;
        t1.gateReqOut --> macDisp.gateReqIn[1];
        macDisp.gateIndOut[1] --> t1.gateIndIn;
        t2.gateReqOut --> nwkDisp.gateReqIn[1];
        nwkDisp.gateIndOut[1] --> t2.gateIndIn;

        ca.gateReqOut --> macDisp.gateReqIn[2];
        macDisp.gateIndOut[2] --> ca.gateIndIn;

        nis.gateIndOut --> macDisp.gateIndIn;
        macDisp.gateReqOut --> nis.gateReqIn;

        nis.nniReqOut --> tcp.gateReqIn;
        tcp.gateIndOut --> nis.nniIndIn;

        nis.cniReqOut --> mac.gateReqIn;
        mac.gateIndOut --> nis.cniIndIn;

        nwkDisp.gateIndOut[2] --> otap.gateIndIn;
        otap.gateReqOut --> nwkDisp.gateReqIn[2];
}

