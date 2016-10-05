package cometos.src.communication.ipv6.nodes;

import cometos.src.communication.addressing.omnetpp.MinParamAddressing;
import cometos.src.communication.ieee802154.mac.CsmaMac;
import cometos.src.communication.ieee802154.phy.EmpiricPhyNic;
import cometos.src.communication.ieee802154.phy.StaticLocation;
import cometos.src.communication.ipv6.coap.CoAPLayer;
import cometos.src.communication.ipv6.coap.CoAPTest;
import cometos.src.communication.ipv6.icmp.ICMPv6;
import cometos.src.communication.ipv6.icmp.NeighborDiscovery;
import cometos.src.communication.ipv6.ipFwd.IPv6InterfaceTable;
import cometos.src.communication.ipv6.ipFwd.IpForward;
import cometos.src.communication.ipv6.lowpan.LowpanAdaptionLayer;
import cometos.src.communication.ipv6.lowpan.LowpanDispatcher;
import cometos.src.communication.ipv6.routing.RoutingTable;
import cometos.src.communication.ipv6.routing.StaticRouting;
import cometos.src.communication.ipv6.udp.UDPLayer;

import org.mixim.base.modules.*;

module CoAP_Node like IWirelessNode
{
    parameters:
        int id = default(0);

        @display("bgb=497,491");
    gates:
        input radioIn; // to receive AirFrames

    submodules:
        low: LowpanAdaptionLayer {
            @display("p=188,272");
        }
        nd: NeighborDiscovery {
            @display("p=101,190");
        }
        mac: CsmaMac {
            @display("p=188,378");
        }
        nic: EmpiricPhyNic {
            @display("p=188,448");
        }

        sL: StaticLocation {
            @display("p=425,355");
        }
        mPA: MinParamAddressing {
            @display("p=425,281");
        }

        bu: BaseUtility {
            @display("p=425,428");
        }

        rt: RoutingTable {
            @display("p=276,190");
        }

        rm: StaticRouting {
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
        CoAPTest: CoAPTest {
            @display("p=188,28");
        }
        CoAP: CoAPLayer {
            @display("p=276,28");
        }
        lD: LowpanDispatcher {
            @display("p=101,326");
        }
    connections:
        mac.lowerControlOut --> nic.upperControlIn;
        mac.lowerGateOut --> nic.upperGateIn;
        nic.upperControlOut --> mac.lowerControlIn;
        nic.upperGateOut --> mac.lowerGateIn;
        nic.radioIn <-- radioIn;
        ip.toLowpan --> low.fromIP;
        low.toIP --> ip.fromLowpan;
        icm.toIP --> ip.fromICMP;
        ip.toICMP --> icm.fromIP;
        udp.toIP --> ip.fromUDP;
        ip.toUDP --> udp.fromIP;
        low.toMAC --> lD.LowpanIn;
        lD.LowpanOut --> low.fromMAC;
        lD.gateReqOut --> mac.gateReqIn;
        mac.gateIndOut --> lD.gateIndIn;
}
