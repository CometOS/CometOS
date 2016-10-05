package cometos.src.communication.ieee802154.mac.omnetpp.mixim;

import org.mixim.base.modules.BaseMobility;

simple StaticConcentricMobility extends BaseMobility
{
    parameters:
        @class(StaticConcentricMobility);

        int numHosts;
        double distance;
}

