package cometos.src.communication.routing;

simple LocationBasedRouting extends Routing
{
    parameters:
        @class(cometos::LocationBasedRouting);
        int slotDuration=default(5000);
		bool isSink=default(false);
		int slotsToRun=default(-1);
	gates:
        input beaconIn;
        output beaconOut;
        
}



