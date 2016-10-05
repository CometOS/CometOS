package cometos.src.communication.transformation;

import cometos.src.communication.base.Layer;

simple LWT extends Layer
{
    parameters:
        @class(cometos::LWT);
       
    gates:
        input gateControlIn;
        output gateControlOut;
    
}
