package cometos.src.communication.routing;
simple GreedyLocationBasedRouting extends Routing
{
    parameters:
        @class(cometos::GreedyLocationBasedRouting);
        int slotDuration @unit(ms) = default(50s);
    gates:
        input beaconIn;
        output beaconOut;
}



