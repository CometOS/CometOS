package cometos.src.communication.base;

import cometos.src.core.Module;


simple Dispatcher extends Module
{
      
        @class(cometos::Dispatcher);
    gates:
        input gateIndIn;
        output gateReqOut;
        
        input gateReqIn[8] @loose;
        output gateIndOut[8] @loose;
}
