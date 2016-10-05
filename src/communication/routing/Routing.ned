package cometos.src.communication.routing;
import cometos.src.communication.base.Layer;

simple Routing extends Layer
{
    parameters:
        @class(Routing);
        int sendingOffset @unit(ms) = default(50ms);
        int maxHops = default(255);
        @display("i=block/routing");
}



