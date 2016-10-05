package cometos.src.communication.ipv6.networks;

import cometos.src.communication.ieee802154.phy.StaticConnectionManager;
import cometos.src.communication.ipv6.nodes.IWirelessNode;
import org.mixim.base.modules.*;

network Network
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        double numNodes; // total number of hosts in the network
        string nodeType = default("Lowpan_Node");

        @display("bgb=$playgroundSizeX,$playgroundSizeY,white;bgp=0,0");
    submodules:
        connectionManager: StaticConnectionManager {
            parameters:
                @display("p=24,21;b=42,42,rect,green;i=abstract/multicast");
        }
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=15,178;i=misc/globe");
        }
        node[numNodes]: <nodeType> like IWirelessNode {
            parameters:
                @display("p=89,101");
        }
    connections allowunconnected:
}

