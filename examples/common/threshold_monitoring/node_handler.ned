package cometos.examples.common.threshold_monitoring;

import cometos.src.communication.base.Endpoint;

simple node_handler extends Endpoint
{
    parameters:
        @class(cometos::node_handler);
        
    	bool initiator = default(false);
        int slf_msg_timer @unit(ms) = default(0ms);
        int start_delay @unit(ms) = default(50ms);
        int threshold = default(20);
}
