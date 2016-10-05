package cometos.src.communication.ieee802154.mac;

import org.mixim.modules.phy.*;

module Ieee154Phy
{
    gates:
        input upperGateIn; // from upper layers
        input radioIn; // to receive AirFrames
        output upperGateOut; // to upper layers
        output upperControlOut; // control connection
        input upperControlIn;


    submodules:
        phy: PhyLayer {
            parameters:
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
                @display("p=82,85;i=block/wrxtx");
                @display("p=115,81");
        }

    connections:
        radioIn --> phy.radioIn;

        phy.upperGateOut --> upperGateOut;
        phy.upperControlOut --> upperControlOut;
        phy.upperGateIn <-- upperGateIn;
        phy.upperControlIn <-- upperControlIn;
}
