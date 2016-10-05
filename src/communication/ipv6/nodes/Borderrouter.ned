package cometos.src.communication.ipv6.nodes;

import cometos.src.communication.ieee802154.mac.CsmaMac;
import cometos.src.communication.ipv6.ipFwd.IpForward;
import cometos.src.communication.ipv6.lowpan.LowpanAdaptionLayer;
import cometos.src.communication.ipv6.lowpan.LowpanDispatcher;
import cometos.src.communication.routing.SimpleRouting;
import cometos.src.communication.tcp.TcpComm;


module tempTest like IWirelessNode
{
    parameters:
        int id = default(0);

        @display("bgb=590,561");
    gates:
        input radioIn; // to receive AirFrames

    submodules:
        // -exchange: TcpComm with SerialComm
        comm: TcpComm {
            @display("p=63,373;i=block/socket");
        }
        routing: SimpleRouting {
            @display("p=163,473");
        }
        low: LowpanAdaptionLayer {
            @display("p=263,373");
        }
        ip: IpForward {
            @display("p=363,373");
        }
        br: CsmaMac {
            @display("p=463,373");
        }

        lD: LowpanDispatcher {
            @display("p=163,373");
        }
    connections:
        low.toIP --> ip.fromLowpan;
        ip.toLowpan --> low.fromIP;

        br.gateIndOut --> ip.fromICMP;
        ip.toICMP --> br.gateReqIn;

        low.toMAC --> lD.LowpanIn;
        lD.LowpanOut --> low.fromMAC;
        comm.gateIndOut --> lD.gateIndIn;
        lD.gateReqOut --> comm.gateReqIn;
        lD.gateIndOut[0] --> routing.gateIndIn;
        routing.gateReqOut --> lD.gateReqIn[0];
}
