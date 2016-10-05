package cometos.src.communication.routing;

simple ShortCuttingTreeRouting extends Routing
{
    parameters:
        @class(ShortCuttingTreeRouting);
        int slotDuration=default(5000);
		bool isSink=default(false);
		int slotsToRun=default(-1);
		int broadcastTxPower=default(0);
	gates:
        input beaconIn;
        output beaconOut;

}



