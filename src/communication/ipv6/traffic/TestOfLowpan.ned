package cometos.src.communication.ipv6.traffic;

import cometos.src.communication.ipv6.icmp.NeighborDiscovery;
import cometos.src.communication.ipv6.lowpan.LoWpanTest;
import cometos.src.communication.ipv6.lowpan.LowpanAdaptionLayer;


network TestOfLowpan
{
    parameters:
        int id = default(0);

        @display("bgb=376,403");
    submodules:
        tst: LoWpanTest {
            @display("p=61,87");
        }
        ada: LowpanAdaptionLayer {
            @display("p=61,201");
        }
        bnc: Bounce {
            @display("p=61,317");
        }
        nd: NeighborDiscovery {
            @display("p=160,87");
        }
    connections:
        ada.toIP --> tst.fromIP;
        ada.toMAC --> bnc.MACinput;
        tst.toIP --> ada.fromIP;
        bnc.MACoutput --> ada.fromMAC;
}
