package cometos.src.communication.routing;
import cometos.src.communication.base.Layer;

simple DHRouting extends Layer
{
    parameters:
        @class(DHRouting);
        @display("i=block/routing");
		double maxDiscoveryTime  @unit(s) = default(60s);
    gates:

        input discoveryIndIn;
        output discoveryReqOut;

        input nbUpdateIndIn;
        output nbUpdateReqOut;

}



