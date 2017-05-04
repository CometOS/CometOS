package cometos.examples.common.tictoc;

import cometos.src.communication.base.Endpoint;

simple node_handler extends Endpoint
{
    parameters:
        @class(cometos::node_handler);
        
    	bool initiator = default(false);
        
        
        int dst = default(-1);
        int interval @unit(ms) = default(0ms);
        int payloadSize = default(50);
        int start @unit(ms) = default(50ms);
}
