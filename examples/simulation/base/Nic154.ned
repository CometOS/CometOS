package cometos.examples.simulation.base;

import cometos.src.communication.base.ILowerEndpoint;
import cometos.src.communication.ieee802154.mac.IMacAbstractionLayer;
import org.mixim.modules.phy.PhyLayer;
import org.mixim.base.modules.BaseUtility;

import org.mixim.modules.phy.*;

module Nic154 like ILowerEndpoint
{
    parameters:
        string macType = default("cometos.src.communication.ieee802154.mac.CsmaMac");
        int id;

        @display("bgb=84,195,lightBlue");
    gates:
        input gateReqIn;
        output gateIndOut;

        output gateSnoopIndOut @loose;

        input radioIn; // radioIn gate for sendDirect
    submodules:
        phy: PhyLayer {
            parameters:
                coreDebug = default(false);
                decider = default(xmldoc("config.xml"));
                analogueModels = default(xmldoc("config.xml"));
                //publishRSSIAlways = false;
                sensitivity = -100 dBm;
                headerLength = 48 bit; // ieee 802.15.4
                thermalNoise = -100 dBm;
                maxTXPower = 2.2387 mW;
                initialRadioState = 0;
                // From TI CC1100 datasheet rev. C
                timeSleepToRX = 0.00108 s;
                timeSleepToTX = 0.00108 s;
                timeRXToTX = 0.000010 s;
                timeTXToRX = 0.000022 s;
                timeRXToSleep = 0 s;
                timeTXToSleep = 0 s;
                @display("p=44,147;i=block/wrxtx");
                @display("p=115,81");

                usePropagationDelay = false;
                useThermalNoise = false;

        }


        mac: <macType> like IMacAbstractionLayer {
            macMinBE = default(5);
            macMaxBE = default(8);
            macMaxCSMABackoffs = default(4);
            macMaxFrameRetries = default(5);
            @display("p=44,36;i=block/mac");
        }
    connections:

        radioIn --> phy.radioIn;

        mac.lowerControlOut --> phy.upperControlIn;
        phy.upperControlOut --> mac.lowerControlIn;

        phy.upperGateOut --> mac.lowerGateIn;
        mac.lowerGateOut --> phy.upperGateIn;

        mac.gateIndOut --> gateIndOut;
        gateReqIn --> mac.gateReqIn;

        mac.gateSnoopIndOut --> gateSnoopIndOut;

}
