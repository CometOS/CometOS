package cometos.src.communication.traffic;

import cometos.src.communication.base.Endpoint;

simple PeriodicTraffic extends Endpoint
{
    parameters:
        @class(cometos::PeriodicTraffic);

        int dst = default(-1);
        int interval @unit(ms) = default(0ms);
        int payloadSize = default(50);
        int start @unit(ms) = default(50ms);

}
