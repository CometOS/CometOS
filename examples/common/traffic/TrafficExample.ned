package cometos.examples.common.mac;

import cometos.src.communication.base.Endpoint;

simple TrafficExample extends Endpoint
{
    parameters:
        string destinations = default("0xFFFF");
        bool snoop = default(false);
        int msgSize = default(24);
        int fixedInterval @unit(ms);
        int rndInterval @unit(ms);
        int timeLimitMS @unit(ms) = default(0s);
        @class(cometos::TrafficExample);
        
    gates:
        input gateSnoopIn;
}
