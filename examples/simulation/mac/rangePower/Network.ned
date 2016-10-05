package cometos.examples.simulation.mac.rangePower;

import cometos.examples.simulation.base.BaseNetwork;

network Network extends BaseNetwork
{
    parameters:
        int numNodes=100; // total number of hosts in the network
        @display("bgb=32,8.3");
    submodules:
        node[numNodes]: Node {
            parameters:
                id = index; // first id 
                mobType="org.mixim.base.modules.BaseMobility";
                nic.macType = "cometos.communication.ieee802154.mac.SimpleCsmaMac";
                mobility.x=index;
                mobility.y=0;
                @display("p=25.1,3.4;r=40;i=misc/node2;is=s");
        }


}

