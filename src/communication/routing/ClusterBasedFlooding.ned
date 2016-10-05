package cometos.src.communication.routing;
simple ClusterBasedFlooding extends Routing
{
    parameters:
        // if true, node always particpate in message forwarding
        bool dom = default(false);
        bool force = default(false);

        @class(ClusterBasedFlooding);
}



