package cometos.src.communication.power;
import cometos.src.communication.base.Layer;

simple PowerAdaptation extends Layer
{
    parameters:
        @class(cometos::PowerAdaptation );
        // set tx power of broadcasts to txMin
        int broadcastTxPower=default(0);
        @display("i=block/routing");
}



