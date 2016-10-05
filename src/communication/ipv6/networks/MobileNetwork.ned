package cometos.src.communication.ipv6.networks;

import cometos.src.communication.ipv6.nodes.MobileNode;
import cometos.examples.simulation.base.BaseNetwork;


network MobileNetwork extends BaseNetwork
{
    parameters:
        string nodeType;
        int numNodes; // total number of hosts in the network
        @display("bgb=32,8.3");
    submodules:
        node[numNodes]: <nodeType> like MobileNode {
            parameters:
                id = default(index); // first id 
                @display("p=25.1,3.4;r=40;i=misc/node2;is=s");
        }


}

