package cometos.examples.simulation.distributed;

network Network
{
    parameters:
        @display("bgb=218,171,white;bgp=0,0");
    submodules:
        node1: Node {
            @display("p=60,40");
        }
        node2: Node {
            @display("p=151,40");
        }
        node3: Node {
            @display("p=60,110");
        }
        node4: Node {
            @display("p=151,110");
        }
    connections allowunconnected:

}

