package cometos.src.communication.ipv6.lowpan;

import cometos.src.core.Module;


simple LowpanDispatcher extends Module
{
      
        @class(cometos_v6::LowpanDispatcher);
    gates:
        input gateIndIn;
        output gateReqOut;
        
        input gateReqIn[8] @loose;
        output gateIndOut[8] @loose;

        input LowpanIn;
        output LowpanOut;
}
