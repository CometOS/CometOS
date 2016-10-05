package cometos.src.communication.ipv6.nodes;

import cometos.src.communication.addressing.omnetpp.MinParamAddressing;
import cometos.src.communication.base.Dispatcher;
import cometos.src.communication.base.RemoteAccess;
import cometos.src.communication.ieee802154.mac.CsmaMac;
import cometos.src.communication.ieee802154.mac.NetworkInterfaceSwitch;
import cometos.src.communication.ieee802154.phy.EmpiricPhyNic;
import cometos.src.communication.ieee802154.phy.StaticLocation;
import cometos.src.communication.ipv6.icmp.ICMPv6;
import cometos.src.communication.ipv6.icmp.NeighborDiscovery;
import cometos.src.communication.ipv6.ipFwd.IPv6InterfaceTable;
import cometos.src.communication.ipv6.ipFwd.IpForward;
import cometos.src.communication.ipv6.lowpan.LowpanAdaptionLayer;
import cometos.src.communication.ipv6.lowpan.LowpanDispatcher;
import cometos.src.communication.ipv6.routing.RoutingTable;
import cometos.src.communication.ipv6.routing.StaticRouting;
import cometos.src.communication.ipv6.traffic.BorderRouterTest;
import cometos.src.communication.ipv6.udp.UDPLayer;
import cometos.src.communication.routing.SimpleRouting;
import cometos.src.communication.systemmonitor.SystemMonitor;
import cometos.src.communication.tcp.TcpComm;

import org.mixim.base.modules.*;

module real like IWirelessNode
{
    parameters:
        int id = default(0);

        @display("bgb=590,561");
    gates:
        input radioIn; // to receive AirFrames

    submodules:
        // -statusLED

        sys: SystemMonitor {
            @display("p=535,277");
        }

        // -leaveOut: StaticLocation
        sL: StaticLocation {
            @display("p=535,125");
        }
        // -leaveOut: MinParamAddressing
        mPA: MinParamAddressing {
            @display("p=535,58");
        }
        // -leaveOut: BaseUtility
        bu: BaseUtility {
            @display("p=535,200");
        }

        // -leaveOut: EmpiricPhyNic
        nic: EmpiricPhyNic {
            @display("p=161,509");
        }
        mac: CsmaMac {
            @display("p=161,439");
        }
        // -exchange: TcpComm with SerialComm
        tcp: TcpComm {
            @display("p=83,439;i=block/socket");
        }
        // -setParameter: NetworkInterfaceSwitch: "nis",0x0100,0xFFFE
        nis: NetworkInterfaceSwitch {
            @display("p=122,373");
        }
        routing: SimpleRouting {
            @display("p=322,373");
        }
        nwkDisp: Dispatcher {
            @display("p=409,373");
        }
        remoteAccess: RemoteAccess {
            @display("p=496,373");
        }

        low: LowpanAdaptionLayer {
            @display("p=232,277");
        }
        ip: IpForward {
            @display("p=232,201");
        }
        icm: ICMPv6 {
            @display("p=320,108");
        }
        udp: UDPLayer {
            @display("p=232,108");
        }
        nd: NeighborDiscovery {
            @display("p=44,174");
        }
        rt: RoutingTable {
            @display("p=44,108");
        }
        rm: StaticRouting {
            @display("p=303,201");
        }
        it: IPv6InterfaceTable {
            @display("p=44,245");
        }

        brt: BorderRouterTest {
            @display("p=232,43");
        }

        lD: LowpanDispatcher {
            @display("p=231,372");
        }
    connections:
        radioIn --> nic.radioIn;

        nic.upperControlOut --> mac.lowerControlIn;
        nic.upperGateOut --> mac.lowerGateIn;
        mac.lowerControlOut --> nic.upperControlIn;
        mac.lowerGateOut --> nic.upperGateIn;

        mac.gateIndOut --> nis.cniIndIn;
        nis.cniReqOut --> mac.gateReqIn;

        tcp.gateIndOut --> nis.nniIndIn;
        nis.nniReqOut --> tcp.gateReqIn;

        routing.gateIndOut --> nwkDisp.gateIndIn;
        nwkDisp.gateReqOut --> routing.gateReqIn;

        nwkDisp.gateIndOut[0] --> remoteAccess.gateIndIn;
        remoteAccess.gateReqOut --> nwkDisp.gateReqIn[0];

        low.toIP --> ip.fromLowpan;
        ip.toLowpan --> low.fromIP;

        ip.toICMP --> icm.fromIP;
        icm.toIP --> ip.fromICMP;

        ip.toUDP --> udp.fromIP;
        udp.toIP --> ip.fromUDP;

        low.toMAC --> lD.LowpanIn;
        lD.LowpanOut --> low.fromMAC;
        nis.gateIndOut --> lD.gateIndIn;
        lD.gateReqOut --> nis.gateReqIn;
        routing.gateReqOut --> lD.gateReqIn[0];
        lD.gateIndOut[0] --> routing.gateIndIn;
}
