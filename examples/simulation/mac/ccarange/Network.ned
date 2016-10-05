package cometos.examples.simulation.mac.ccarange;

import cometos.examples.simulation.base.BaseNetwork;


network Network extends BaseNetwork
{
    parameters:
        double numNodes; // total number of hosts in the network

        @display("bgb=239.6,181.4,white;bgp=0,0");
    submodules:
        node[numNodes]: Node {
            parameters:

                mobility.y = default(50);
                mobility.z = 0;
                id = index;

                @display("p=150,95;i=block/circle");
        }
    connections allowunconnected:

}

