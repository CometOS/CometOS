package cometos.examples.simulation.base;

import cometos.src.communication.ieee802154.mac.IMacAbstractionLayer;
import cometos.src.core.omnetpp.SchedulerModule;

import org.mixim.modules.phy.*;
import org.mixim.base.modules.*;
import org.mixim.base.modules.IBaseMobility;

module Node154Flat like INode
{
    parameters:
        int id;
        string macType = default("cometos.communication.ieee802154.mac.SimpleCsmaMac");

        string mobType = default("BaseMobility"); //type of the mobility module
    gates:
        input radioIn; // to receive AirFrames
    submodules:
        mobility: <mobType> like IBaseMobility {
            parameters:
                coreDebug = default(false);
                z = default(0);
                @display("p=251,380;i=block/cogwheel");
        }

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
                useThermalNoise = true;

        }


        mac: <macType> like IMacAbstractionLayer {
            macMinBE = default(5);
            macMaxBE = default(8);
            macMaxCSMABackoffs = default(4);
            macMaxFrameRetries = default(5);
            @display("p=44,36;i=block/mac");
        }

        utility: BaseUtility {
            parameters:
                coreDebug = default(false);
                @display("p=251,450;b=24,24,rect,black;i=block/blackboard");
        }

        scheduler: SchedulerModule {
            @display("p=251,313;i=device/cpu");
        }
    connections:
        radioIn --> phy.radioIn;
        mac.lowerControlOut --> phy.upperControlIn;
        phy.upperControlOut --> mac.lowerControlIn;

        phy.upperGateOut --> mac.lowerGateIn;
        mac.lowerGateOut --> phy.upperGateIn;

}
