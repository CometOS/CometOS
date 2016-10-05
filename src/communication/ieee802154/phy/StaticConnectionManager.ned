package cometos.src.communication.ieee802154.phy;

simple StaticConnectionManager
{
    parameters:
        // debug switch for core framework
        bool coreDebug;
        // send directly to the node or create separate gates for every connection
        bool sendDirect;
        // minimum carrier frequency of the channel [Hz]
        double carrierFrequency @unit(Hz);
        // link statistics
        xml connectionsFile;
        @display("i=abstract/multicast");
}
