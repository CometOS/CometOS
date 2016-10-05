package cometos.src.communication.base;

import cometos.src.core.Module;


simple Endpoint extends Module
{
   	parameters:
        @class(cometos::Endpoint);
    gates:
        input gateIndIn;
        output gateReqOut;
}
