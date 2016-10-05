package cometos.src.communication.ipv6.routing;

simple StaticRouting
{
    parameters:
        xml routingFile;

        @class(cometos_v6::StaticRouting);
        @display("i=block/routing");
}
