package cometos.src.communication.routing;

simple SectorLocationBasedRouting extends Routing
{
    parameters:
        @class(cometos::SectorLocationBasedRouting);
        int slotDuration @unit(ms) = default(50s);
    gates:
        input sectorRequestIn;
        output sectorRequestOut;
        input sectorReplyIn;
        output sectorReplyOut;

}



