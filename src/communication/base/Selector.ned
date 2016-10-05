package cometos.src.communication.base;

import cometos.src.core.Module;


simple Selector extends Module
{
    @class(cometos::Selector);
    gates:
        input gateIndIn[8] @loose;
        output gateReqOut[8] @loose;
        input gateReqIn;
        output gateIndOut;
}
