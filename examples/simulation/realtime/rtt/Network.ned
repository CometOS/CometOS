package cometos.examples.simulation.realtime.rtt;

import org.mixim.base.modules.*;
import org.mixim.base.connectionManager.*;

network Network
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        double numNodes; // total number of hosts in the network

        @display("bgb=552,395,white;bgp=0,0");
    submodules:
        connectionManager: ConnectionManager {
            parameters:
                @display("p=30,27;b=42,42,rect,green;i=abstract/multicast");
        }
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=28,92;i=misc/globe");
        }
        node[numNodes]: Node {
            parameters:
                id = index+1;
                @display("p=141,92;r=90;i=misc/node2;is=s");
        }
    connections allowunconnected:

}

