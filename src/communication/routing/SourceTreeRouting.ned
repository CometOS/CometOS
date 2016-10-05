package cometos.src.communication.routing;
import cometos.src.core.Module;

simple SourceTreeRouting extends Module
{
    parameters:
        bool isSink = default(false);
        @class(SourceTreeRouting);
        int sendingOffset @unit(ms) =  default(100ms);
    gates:
        input sourceIndIn;
        output sourceReqOut;

        input treeIndIn;
        output treeReqOut;

        input innerIndIn;
        output innerReqOut;
        input innerReqIn;
        output innerIndOut;
        
        input  gateReqIn;
        output gateIndOut;
}



