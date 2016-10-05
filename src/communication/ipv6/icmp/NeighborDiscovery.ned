package cometos.src.communication.ipv6.icmp;

simple NeighborDiscovery
{
    parameters:
        @class(cometos_v6::NeighborDiscovery);
        bool autoInsert = default(true);

        @display("i=block/network");
}
