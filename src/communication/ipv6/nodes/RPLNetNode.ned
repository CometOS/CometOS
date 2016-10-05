package cometos.src.communication.ipv6.nodes;

import cometos.src.communication.addressing.omnetpp.MinParamAddressing;
import cometos.src.communication.base.Dispatcher;
import cometos.src.communication.base.RemoteAccess;
import cometos.src.communication.ieee802154.mac.CsmaMac;
import cometos.src.communication.ieee802154.phy.EmpiricPhyNic;
import cometos.src.communication.ieee802154.phy.StaticLocation;
import cometos.src.communication.ipv6.coap.CoAPLayer;
import cometos.src.communication.ipv6.icmp.ICMPv6;
import cometos.src.communication.ipv6.icmp.NeighborDiscovery;
import cometos.src.communication.ipv6.ipFwd.IPv6InterfaceTable;
import cometos.src.communication.ipv6.ipFwd.IpForward;
import cometos.src.communication.ipv6.ipFwd.UnknownProtocol;
import cometos.src.communication.ipv6.lowpan.LowpanAdaptionLayer;
import cometos.src.communication.ipv6.lowpan.LowpanDispatcher;
import cometos.src.communication.ipv6.routing.RPL;
import cometos.src.communication.ipv6.routing.RPL.RPLRoutingTable;
import cometos.src.communication.ipv6.udp.UDPLayer;
import cometos.src.communication.reliability.SimpleReliabilityLayer;
import cometos.src.communication.routing.SimpleRouting;
import cometos.src.communication.systemmonitor.SystemMonitor;

import org.mixim.base.modules.*;

module RPLNetNode like IWirelessNode
{
    parameters:
        int id = default(0);
        //DIO - CONFIG
        bool authentificationEnable = default(false);

        // keep consistent with RPLRouting.h
        int defaultDioIntervalMin = default(5);
        int defaultDioIntervalDoublings = default(20);
        int defaultDioRedundancyConst = default(10);
        int defaultDioMinHopRankIncrease = default(256);

        bool defaultDioRoot = default(false);
        bool grounded = default(false);

        @display("bgb=590,561");
    gates:
        input radioIn; // to receive AirFrames

    submodules:
        // -statusLED

        sys: SystemMonitor {
            @display("p=535,190");
        }

        low: LowpanAdaptionLayer {
            @display("p=188,260");
        }
        nd: NeighborDiscovery {
            @display("p=101,190");
        }
        mac: CsmaMac {
            @display("p=188,460");
        }
        // -exchange: TcpComm with SerialComm
        //        tcp: LowerEndpoint {
        //            @display("p=126,394;i=block/socket");
        //        }
        //        // -setParameter: NetworkInterfaceSwitch: "nis",0x0100,0xFFFE
        //        nis: NetworkInterfaceSwitch {
        //            @display("p=188,394");
        //        }
        // -leaveOut: EmpiricPhyNic
        nic: EmpiricPhyNic {
            @display("p=188,524");
        }
		// -leaveOut: StaticLocation
        sL: StaticLocation {
            @display("p=535,97");
        }
        // -leaveOut: MinParamAddressing
        mPA: MinParamAddressing {
            @display("p=535,28");
        }
		// -leaveOut: BaseUtility
        bu: BaseUtility {
            @display("p=535,260");
        }

        rt: RPLRoutingTable {
            @display("p=276,190");
        }

        rm: RPL {
            @display("p=340,190");
        }
        it: IPv6InterfaceTable {
            @display("p=43,190");
        }
        ip: IpForward {
            @display("p=188,190");
        }
        icm: ICMPv6 {
            @display("p=276,97");
        }
        udp: UDPLayer {
            @display("p=188,97");
        }
        up: UnknownProtocol {
            @display("p=101,97");
        }
        lD: LowpanDispatcher {
            @display("p=188,326");
        }
        CoAP: CoAPLayer {
            @display("p=276,28");
        }

        routing: SimpleRouting {
            @display("p=276,326");
        }
        nwkDisp: Dispatcher {
            @display("p=351,326");
        }
        remoteAccess: RemoteAccess {
            @display("p=496,326");
        }
        srl: SimpleReliabilityLayer {
            @display("p=422,326");
        }
        // -setNamespace: Multitest::None
        //brt: BorderRouterTest {
          //  @display("p=364,28");
        //}
    connections:
        //up.toIP --> ip.fromUnknown;
        ip.toUnknown --> up.fromIP;

        udp.toIP --> ip.fromUDP;
        ip.toUDP --> udp.fromIP;

        icm.toIP --> ip.fromICMP;
        ip.toICMP --> icm.fromIP;

        ip.toLowpan --> low.fromIP;
        low.toIP --> ip.fromLowpan;

        lD.LowpanOut --> low.fromMAC;
        low.toMAC --> lD.LowpanIn;

        mac.gateIndOut --> lD.gateIndIn;
        lD.gateReqOut --> mac.gateReqIn;

        routing.gateIndOut --> nwkDisp.gateIndIn;
        nwkDisp.gateReqOut --> routing.gateReqIn;

        nwkDisp.gateIndOut[0] --> srl.gateIndIn;
        srl.gateReqOut --> nwkDisp.gateReqIn[0];

        srl.gateIndOut --> remoteAccess.gateIndIn;
        remoteAccess.gateReqOut --> srl.gateReqIn;

        routing.gateReqOut --> lD.gateReqIn[0];
        lD.gateIndOut[0] --> routing.gateIndIn;

        mac.lowerControlOut --> nic.upperControlIn;
        mac.lowerGateOut --> nic.upperGateIn;
        nic.upperControlOut --> mac.lowerControlIn;
        nic.upperGateOut --> mac.lowerGateIn;

        nic.radioIn <-- radioIn;
}
