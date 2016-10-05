package cometos.examples.simulation.base;


import cometos.src.auxiliary.ExtendedWorldUtility;

import org.mixim.base.modules.*;
import org.mixim.base.connectionManager.IConnectionManager;

network BaseNetwork
{
    parameters:
        string cmType = default("org.mixim.base.connectionManager.ConnectionManager");

        double playgroundSizeX @unit(m) = default(4500m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m) = default(4500m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m) = default(1m); // z size of the area the nodes are in (in meters)

        @display("bgb=21.4,9,#A7D8C4;bgp=0,0;bgs=10");

    submodules:

        connectionManager: <cmType> like IConnectionManager {
            parameters:
                pMax = default(1mW);
                sat = default(-97dBm);
                alpha = default(3.0);
                //Network.connectionManager.pMax = 100mW
				//Network.connectionManager.sat = -84dBm
                carrierFrequency = default(2.412e+9Hz);
                @display("i=abstract/multicast;p=13.9,3.4");
        }
        //world: <wuType> like IWorldUtility {
        world: ExtendedWorldUtility {
            parameters:
                coreDebug = false;
                useTorus = default(false);
                use2D = default(true);

                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("i=misc/globe;p=3.8,3.4");
        }
    connections allowunconnected:
}
