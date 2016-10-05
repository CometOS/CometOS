package cometos.src.communication.routing;
simple TreeRouting extends Routing
{
    parameters:
        @class(cometos::TreeRouting);
        int slotDuration = default(5000);
        bool isSink = default(false);
        int slotsToRun = default(-1);
    gates:
        input beaconIn;
        output beaconOut;

}



