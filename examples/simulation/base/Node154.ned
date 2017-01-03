package cometos.examples.simulation.base;

import cometos.src.core.omnetpp.SchedulerModule;
import org.mixim.base.modules.IBaseMobility;
import org.mixim.base.modules.BaseUtility;

import org.mixim.base.modules.*;

module Node154 like INode
{
    parameters:
        int id;
        nic.id = id;

        string mobType = default("BaseMobility"); //type of the mobility module
    gates:
        input radioIn; // to receive AirFrames
    submodules:
        mobility: <mobType> like IBaseMobility {
            parameters:
                coreDebug = default(false);
                z = default(0);
                @display("p=251,380;i=block/cogwheel");
        }

        nic: Nic154 {
            @display("p=47,450");
        }
        utility: BaseUtility {
            parameters:
                coreDebug = default(false);
                @display("p=251,450;b=24,24,rect,black;i=block/blackboard");
        }

        scheduler: SchedulerModule {
            @display("p=251,313;i=device/cpu");
        }
        
        location: LocationModule {
        }
        
    connections:
        radioIn --> nic.radioIn;
}
