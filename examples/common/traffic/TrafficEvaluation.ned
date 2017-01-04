package cometos.examples.common.traffic;

import cometos.src.communication.base.Endpoint;

simple TrafficEvaluation extends Endpoint
{
    parameters:
        int destination = default(-1);
        int msgSize = default(60);
        int meanInterval @unit(ms);
        int warmupDuration @unit(ms) = default(0ms);
        int cooldownDuration @unit(ms) = default(0ms);
        int maxMeasurementPackets = default(100);
        @class(cometos::TrafficEvaluation);
}
