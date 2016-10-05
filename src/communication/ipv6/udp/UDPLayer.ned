package cometos.src.communication.ipv6.udp;

import cometos.src.core.Module;

simple UDPLayer extends Module
{
    parameters:
        @class(cometos_v6::UDPLayer);
        @display("i=block/socket");
    gates:
        input fromIP;
        output toIP;
}
