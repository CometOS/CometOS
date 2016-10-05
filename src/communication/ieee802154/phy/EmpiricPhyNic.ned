package cometos.src.communication.ieee802154.phy;

module EmpiricPhyNic
{
    parameters:
        @display("i=block/wrxtx");
    gates:
        input upperGateIn; // to upper layers
        output upperGateOut; // from upper layers
        output upperControlOut; // control information 
        input upperControlIn; // control information 
        input radioIn; // radioIn gate for sendDirect

    submodules:
//        mac: CSMAMacLayer {
//            @display("p=96,87;i=block/layer");
//        }

        phy: EmpiricModelPhy {
            @display("p=132,74;i=block/process_s");
        }

    connections:
//        mac.upperGateOut --> { @display("ls=black;m=m,25,50,25,0"); } --> upperGateOut;
//        mac.upperGateIn <-- { @display("ls=black;m=m,15,50,15,0"); } <-- upperGateIn;
//        mac.upperControlOut --> { @display("ls=red;m=m,75,50,75,0"); } --> upperControlOut;
//        mac.upperControlIn <-- { @display("ls=red;m=m,85,0,85,0"); } <-- upperControlIn;

//        phy.upperGateOut --> { @display("ls=black;m=m,25,50,25,0"); } --> mac.lowerGateIn;
//        phy.upperGateIn <-- { @display("ls=black;m=m,15,50,15,0"); } <-- mac.lowerGateOut;
//        phy.upperControlOut --> { @display("ls=red;m=m,75,50,75,0"); } --> mac.lowerControlIn;
//        phy.upperControlIn <-- { @display("ls=red;m=m,85,0,85,0"); } <-- mac.lowerControlOut;

        phy.upperGateOut --> { @display("ls=black;m=m,25,50,25,0"); } --> upperGateOut;
        phy.upperGateIn <-- { @display("ls=black;m=m,15,50,15,0"); } <-- upperGateIn;
        phy.upperControlOut --> { @display("ls=red;m=m,75,50,75,0"); } --> upperControlOut;
        phy.upperControlIn <-- { @display("ls=red;m=m,85,0,85,0"); } <-- upperControlIn;

        radioIn --> phy.radioIn;
}
