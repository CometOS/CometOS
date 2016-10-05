package cometos.examples.simulation.comm;



import cometos.src.communication.base.Dispatcher;
import cometos.examples.common.comm.EndpointExample;
import cometos.examples.common.comm.LayerExample;
import cometos.examples.common.comm.LowerEndpointExample;
import cometos.examples.common.comm.SelectorExample;


network Network
{
    parameters:
        int id = 1;
        @display("bgb=226,382");
    submodules:

        lowerEndpoint: LowerEndpointExample {
            @display("p=80,339");
        }
        endpoint: EndpointExample {
            @display("p=80,28");
        }
        layer0: LayerExample {
            @display("p=41,190");
        }
        dispatcher: Dispatcher {
            @display("p=80,265");
        }
        layer1: LayerExample {
            @display("p=110,190");
        }
        selector: SelectorExample {
            @display("p=80,107");
        }
        layer2: LayerExample {
            @display("p=187,190");
        }
    connections:


        selector.gateIndOut --> endpoint.gateIndIn;
        endpoint.gateReqOut --> selector.gateReqIn;
        selector.gateReqOut[0] --> layer0.gateReqIn;
        layer0.gateIndOut --> selector.gateIndIn[0];
        selector.gateReqOut[1] --> layer1.gateReqIn;
        layer1.gateIndOut --> selector.gateIndIn[1];
        layer2.gateIndOut --> selector.gateIndIn[2];
        selector.gateReqOut[2] --> layer2.gateReqIn;
        layer0.gateReqOut --> dispatcher.gateReqIn[0];
        layer1.gateReqOut --> dispatcher.gateReqIn[1];
        layer2.gateReqOut --> dispatcher.gateReqIn[2];
        dispatcher.gateIndOut[0] --> layer0.gateIndIn;
        dispatcher.gateIndOut[1] --> layer1.gateIndIn;
        dispatcher.gateIndOut[2] --> layer2.gateIndIn;
        dispatcher.gateReqOut --> lowerEndpoint.gateReqIn;
        lowerEndpoint.gateIndOut --> dispatcher.gateIndIn;
}


