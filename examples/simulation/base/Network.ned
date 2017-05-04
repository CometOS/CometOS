package cometos.examples.simulation.base;



network Network extends BaseNetwork
{
    parameters:
        string nodeType;
        int numNodes; // total number of hosts in the network
        @display("bgb=32,8.3");
    submodules:
//                Coordinator: <"cometos.examples.common.tictoc.node_Coordinator"> like INode {
//            parameters:
//                id = 1;
//                @display("p=25.1,3.4;r=40;i=misc/node2;is=s");
//        }
        
        node[numNodes]: <nodeType> like INode {
            parameters:
                id = default(index); // first id 
                @display("p=25.1,3.4;r=40;i=misc/node2;is=s");
        }


}

