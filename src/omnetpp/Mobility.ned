package cometos.src.omnetpp;

import org.mixim.base.modules.IBaseMobility;

// Reads node position from file
simple Mobility like IBaseMobility
{
    parameters:
        xml topo;

        // these vars are not used, but neeed for compatibility
        bool notAffectedByHostState = true;
        bool coreDebug = default(false); // debug switch for the core framework
        double x = default(-1); // x coordinate of the nodes' position (-1 = random)
        double y = default(-1); // y coordinate of the nodes' position (-1 = random)
        double z = default(0); // z coordinate of the nodes' position (-1 = random)


}

