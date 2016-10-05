package cometos.src.communication.traffic;

import cometos.src.communication.base.Endpoint;

simple PoissonTraffic extends Endpoint
{
    parameters:
        @class(cometos::PoissonTraffic);

        int dst = default(-1);
        double interval @unit(ms) = default(0ms);
        int payloadSize = default(50);
        double start @unit(ms) = default(0ms);

}
