package cometos.examples.simulation.mp;



import cometos.examples.common.mp.MyReceiver;
import cometos.examples.common.mp.MySender;


network Network
{
    parameters:
        int id = 1;
        @display("bgb=200,89");
    submodules:

        myReceiver: MyReceiver {
            @display("p=156,35");
        }
        mySender: MySender {
            @display("p=39,35");
        }
    connections:


        mySender.gateOut --> myReceiver.gateIn;
}


