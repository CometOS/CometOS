package cometos.src.communication.ipv6.ipFwd;

import cometos.src.core.Module;

simple UnknownProtocol extends Module
{
    parameters:
        @class(cometos_v6::UnknownProtocol);
        @display("i=block/arrival");
    gates:
        input fromIP;
}
