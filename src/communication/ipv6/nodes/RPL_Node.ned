package cometos.src.communication.ipv6.nodes;

import cometos.src.communication.addressing.omnetpp.MinParamAddressing;
import cometos.src.communication.ieee802154.mac.AtmelRF230SnrLqiProvider;
import cometos.src.communication.ieee802154.mac.CsmaMac;
import cometos.src.communication.ieee802154.phy.EmpiricPhyNic;
import cometos.src.communication.ieee802154.phy.StaticLocation;
import cometos.src.communication.ipv6.icmp.ICMPv6;
import cometos.src.communication.ipv6.icmp.NeighborDiscovery;
import cometos.src.communication.ipv6.ipFwd.IPv6InterfaceTable;
import cometos.src.communication.ipv6.ipFwd.IpForward;
import cometos.src.communication.ipv6.lowpan.LowpanAdaptionLayer;
import cometos.src.communication.ipv6.lowpan.LowpanDispatcher;
import cometos.src.communication.ipv6.routing.RPL;
import cometos.src.communication.ipv6.routing.RoutingTable;
import cometos.src.communication.ipv6.routing.SourceRoutingTable;
import cometos.src.communication.ipv6.traffic.TrafficGenSim;
import cometos.src.communication.ipv6.udp.UDPLayer;
import cometos.src.communication.ipv6.lowpan.ILowpanCongestionControl;

import org.mixim.base.modules.*;

module RPL_Node like IWirelessNode
{
    parameters:
        int id = default(0);
        
        string localCongestionControlType = default("NoCC");

        @display("bgb=497,491");
    gates:
        input radioIn; // to receive AirFrames

    submodules:
        low: LowpanAdaptionLayer {
            @display("p=188,267");
        }
        nd: NeighborDiscovery {
            @display("p=101,190");
        }
        mac: CsmaMac {
            @display("p=188,368");
        }
        nic: EmpiricPhyNic {
            @display("p=188,441");
        }

        sL: StaticLocation {
            @display("p=425,355");
        }
        mPA: MinParamAddressing {
            @display("p=425,281");
        }

        lqiProv: AtmelRF230SnrLqiProvider {
            @display("p=425,210");
        }

        bu: BaseUtility {
            @display("p=425,428");
        }

        rt: RoutingTable {
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
        tg: TrafficGenSim {
            @display("p=188,28");
        }
        srt: SourceRoutingTable {
            @display("p=276,256");
        }

        lD: LowpanDispatcher {
            @display("p=111,320");
        }
        
        locc: <localCongestionControlType> like ILowpanCongestionControl {
            @display("p=350,267");
        }
    connections allowunconnected:
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
