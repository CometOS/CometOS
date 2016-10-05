package cometos.src.communication.ipv6.lowpan;


import cometos.src.core.Module;


simple LoWpanTest extends Module
{
    parameters:
        @class(LoWpanTest);
        bool isSender = default(false);
        int initialDest;                
    gates:
        input	fromIP;
        output	toIP;
}
