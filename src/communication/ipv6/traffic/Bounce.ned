package cometos.src.communication.ipv6.traffic;


import cometos.src.core.Module;


simple Bounce extends Module
{
    parameters:
        @class(Bounce);
        
    gates:
        input	MACinput;
        output	MACoutput;
}
