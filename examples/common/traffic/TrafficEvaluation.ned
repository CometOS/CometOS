package cometos.examples.common.traffic;

import cometos.src.communication.base.Endpoint;

simple TrafficEvaluation extends Endpoint
{
    parameters:
        int destination = default(-1);
        int msgSize = default(60);
        int meanInterval @unit(ms);
        int timeLimitMS @unit(ms) = default(0s);
        @class(cometos::TrafficEvaluation);

    gates:
        input gateSnoopIn;
}
