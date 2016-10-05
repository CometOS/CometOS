package cometos.src.communication.base;

import cometos.src.core.Module;


simple Layer extends Module like ILayer
{
    gates:
        input gateIndIn;
        output gateReqOut;
        input gateReqIn;
        output gateIndOut;
}
