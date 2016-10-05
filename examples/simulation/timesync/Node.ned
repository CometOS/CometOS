package cometos.examples.simulation.timesync;

import cometos.examples.simulation.base.Node154Flat;
import cometos.src.communication.time.TimeSyncService;
import cometos.examples.common.timesync.TimeSyncExample;
import cometos.src.communication.base.Dispatcher;


module Node extends Node154Flat
{
    submodules:
        tss: TimeSyncService {
            @display("p=47,137");
        }

        dispatch: Dispatcher {
            @display("p=47,218");
        }

        example: TimeSyncExample {
            @display("p=185,72");
        }
    connections:
        mac.gateIndOut --> dispatch.gateIndIn;
        dispatch.gateReqOut --> mac.gateReqIn;
        
        dispatch.gateIndOut[0] --> tss.gateInitialIn;
        dispatch.gateReqIn[0] <-- tss.gateInitialOut;
        
        dispatch.gateIndOut[1] --> tss.gateTimestampIn;
        dispatch.gateReqIn[1] <-- tss.gateTimestampOut;
}

