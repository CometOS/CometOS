package cometos.src.communication.routing;

simple ProbabilisticFlooding extends Routing
{
    parameters:
        int prob = default(20);
        @class(ProbabilisticFlooding);
}



