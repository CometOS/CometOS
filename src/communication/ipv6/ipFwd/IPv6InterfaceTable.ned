package cometos.src.communication.ipv6.ipFwd;

import cometos.src.core.Module;

// IPv6InterfaceTable
// Manages IP Addresses of all Interfaces
simple IPv6InterfaceTable extends Module
{
    parameters:
        @class(cometos_v6::IPv6InterfaceTable);

        @display("i=block/table");
        
        string address=default("64:29:30:31:0:0:0:0");
        bool isPrefix=default(true);
}
