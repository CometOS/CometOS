package cometos.src.communication.neighborhood;

import cometos.src.communication.base.Layer;

simple TCPWY extends Layer
{
    parameters:
        @class(cometos::TCPWY);
       
    gates:
        input gateControlIn;
        output gateControlOut;
    
}
