package cometos.examples.simulation.mac.multichannel;

import org.mixim.base.modules.*;
import org.mixim.base.connectionManager.*;

network Network
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        double numNodes; // total number of hosts in the network



        @display("bgb=$playgroundSizeX,$playgroundSizeY,white;bgp=0,0");
    submodules:
        // connectionManager: UnitDisk {
        connectionManager: ConnectionManager {
            parameters:
                @display("p=150,0;b=42,42,rect,green,,;i=abstract/multicast");
        }
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=245,162;i=misc/globe");
        }
        node[numNodes]: Node {
            parameters:
                mobility.z = 0;
                @display("p=150,95;i=block/circle");
        }
    connections allowunconnected:

}

