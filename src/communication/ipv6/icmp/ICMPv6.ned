package cometos.src.communication.ipv6.icmp;

import cometos.src.core.Module;

simple ICMPv6 extends Module
{
    parameters:
        @class(cometos_v6::ICMPv6);
        @signal[arrival](type="int");
        @signal[departure](type="int");
        @statistic[incomming](source="arrival"; record=vector,stats; interpolationmode=none);
        @statistic[outgoing](source="departure"; record=vector,stats; interpolationmode=none);
        @display("i=block/control");
    gates:
        input fromIP;
        output toIP;
}
