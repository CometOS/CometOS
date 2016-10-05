package cometos.examples.simulation.distributed;

import cometos.src.communication.tcp.TcpComm;
import cometos.src.communication.traffic.PoissonTraffic;


module Node
{
    parameters:
        int id = default(0);
        @display("bgb=109,163,white");
    submodules:
        tcp: TcpComm {
            @display("p=62,112");
        }
        traffic: PoissonTraffic {
            @display("p=62,30");
        }
    connections:
        tcp.gateIndOut --> traffic.gateIndIn;
        traffic.gateReqOut --> tcp.gateReqIn;
}

