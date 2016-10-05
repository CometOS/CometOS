package cometos.examples.simulation.aggregation;


import cometos.examples.common.aggregation.ObjectAggregationExample;


network Network
{
    
    parameters:
 		int id = 1;
        @display("bgb=161,93");
    submodules:
        a: ObjectAggregationExample {
            @display("p=37,40");
        }
        b: ObjectAggregationExample {
            @display("p=123,40");
        }
    connections:

        a.gateOut --> b.gateIn;
        b.gateOut --> a.gateIn;
}


