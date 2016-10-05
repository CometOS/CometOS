package cometos.src.communication.transformation;

import cometos.src.communication.base.Layer;

simple RNGT extends Layer
{
    parameters:
        @class(cometos::RNGT);
       
    gates:
        input gateControlIn;
        output gateControlOut;
    
}
