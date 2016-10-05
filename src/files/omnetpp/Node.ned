package cometos.src.files.omnetpp;

import cometos.src.communication.base.Dispatcher;
import cometos.src.communication.reliability.SimpleReliabilityLayer;
import cometos.examples.simulation.base.Node154;
import cometos.src.files.Verifier;
import cometos.src.files.deluge.Deluge;


module Node extends Node154
{
    submodules:
        experiment: DelugeDataDisseminationExperiment {
            @display("p=283,45");
        }

        verifier: Verifier {
            @display("p=47,32");
        }

  //      simpleFileTransfer: SimpleFileTransfer {
  //          @display("p=131,45");
   //     }

        deluge: Deluge {
            @display("p=212,45");
        }

        dispatcher: Dispatcher {
            @display("p=47,142");
        }

   //     routing: SimpleRouting {
   //         @display("p=47,221");
   //     }

        verifierReliablity: SimpleReliabilityLayer {
            @display("p=47,84");
        }

    connections:
        verifier.gateReqOut --> verifierReliablity.gateReqIn;
        verifierReliablity.gateIndOut --> verifier.gateIndIn;

        verifierReliablity.gateReqOut --> dispatcher.gateReqIn[0];
        dispatcher.gateIndOut[0] --> verifierReliablity.gateIndIn;

//        simpleFileTransfer.gateReqOut --> dispatcher.gateReqIn[1];
//        dispatcher.gateIndOut[1] --> simpleFileTransfer.gateIndIn;

        deluge.gateReqOut --> dispatcher.gateReqIn[2];
        dispatcher.gateIndOut[2] --> deluge.gateIndIn;

   //     dispatcher.gateReqOut --> routing.gateReqIn;
   //     routing.gateIndOut --> dispatcher.gateIndIn;

        dispatcher.gateReqOut --> nic.gateReqIn;
        nic.gateIndOut --> dispatcher.gateIndIn;

}

