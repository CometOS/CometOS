package cometos.src.communication.ieee802154.mac;
import cometos.src.communication.base.LowerEndpoint;

simple MacAbstractionLayer extends LowerEndpoint like IMacAbstractionLayer
{
    parameters:
                @class(cometos::MacAbstractionLayer);
                
        double bitrate @unit(bps) = default(250000 bps);
        double txPower @unit(mW) = default(1mW);
        
        // these values are used if MacControl is attached to data packet
        // note that ack from other note is not modified by MacControl.h
        double minTxPower @unit(mW) = default(0.01mW);
        double maxTxPower @unit(mW) = default(5mW);
             
        double ccaDetectionTime @unit(s) = default(0.000128 s); // 8 symbols
        double ccaThreshold @unit(dBm) = default(-95dBm);        
        
       	double aUnitBackoffPeriod @unit(s) = default(0.00032 s);

		// our current ACK is one byte larger than the 802.15.4-ACK, so this 
		// has to be at least one byte "longer" than 802.15.4 minimum
		// 802.15.4: (aUnitBackoffPeriod + ) aTurnaroundTime + phySHRDuration + 6 * phySymbolsPerOctet
		//     where aUnitBackoffPeriod = 20 symbols, aTurnaroundTime = 12 symbols, phySHRDuration = 10 symbols for 2.4GHz, 
		//     6 * phySymbolsPerOctet (includes PHR=1 octets and ACK payload=5 octets) = 12 symbols
		// aUnitBackoffPeriod isn't used in the MiXiM 802.15.4 MAC (only relevant for beacon-enabled PANs and the CAP) 
        // double macAckWaitDuration @unit(s) = default(0.000592 s);  // 802.15.4: 0.00056s  1+12+10+12 symbols
        double macAckWaitDuration @unit(s) = default(0.00070 s);  // need increase of timings
               
        int ackLength @unit(bit) = default(48 bit); // 802.15.4: 40 bit 2FrameControl 1seq 2FCS 
        
        double sifs @unit(s) = default(0.000192 s); // 12 symbols
        
        double aTurnaroundTime @unit(s) = default(0.000192 s); 
              
              
        int macMaxFrameRetries = default(3);      
        int macMaxCSMABackoffs = default(5);
        int macCcaMode = default(0);
        int macMinBE = default(3);
        int macMaxBE = default(8);

		bool txWaitsForBusyRx = default(false);
		
		bool macResetCcaBe = default(true);
		
		bool suppressRxWhileTxPending = default(false);
		
		//bool withFailures = default(false);
		//int failureAt = default(-1);
		//int resumeAt = default(-1);
		
		xml configFile = default(xml("<root/>"));

        @display("i=block/layer");
    gates:
        input lowerControlIn;
        output lowerControlOut;
        input lowerGateIn;
        output lowerGateOut;
        
        output gateSnoopIndOut;
}




