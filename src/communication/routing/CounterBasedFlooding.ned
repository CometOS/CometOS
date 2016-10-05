package cometos.src.communication.routing;
import cometos.src.communication.base.Layer;

simple CounterBasedFlooding extends Layer
{
    parameters:
        @class(CounterBasedFlooding);
        int sendingOffset @unit(ms) = default(50ms);
		int maxCounter = default(1);
        @display("i=block/routing");
}



