package cometos.src.communication.ipv6.ipFwd;

import cometos.src.core.Module;

simple IpForward extends Module
{
    parameters:
        @class(cometos_v6::IpForward);
        @display("i=block/dispatch");
        
        int numRequestsToLower;
        int numIndicationsToUpper;
    gates:
//        // to neighbor discovery module
//        input ndIn;
//        output ndOut;

//        // to ICMP module
//        input icmpIn;
//        output icmpOut;
//        
//        // to transport modules
//        input transportIn[];
//        output transportOut[];

        // to link layer
        input fromLowpan;
        output toLowpan;

        input fromUnknown @loose;
        output toUnknown @loose;

        input fromUDP;
        output toUDP;

        input fromICMP;
        output toICMP;

}
