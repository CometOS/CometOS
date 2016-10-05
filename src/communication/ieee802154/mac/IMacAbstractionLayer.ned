package cometos.src.communication.ieee802154.mac;
//
moduleinterface IMacAbstractionLayer
{
	parameters:    
        double bitrate @unit(bps);
        double txPower @unit(mW);
        
        // these values are used if MacControl is attached to data packet
        // note that ack from other note is not modified by MacControl.h
        double minTxPower @unit(mW) ;
        double maxTxPower @unit(mW);
             
        double ccaDetectionTime @unit(s); // 8 symbols
        double ccaThreshold @unit(dbm);        
        
       	double aUnitBackoffPeriod @unit(s);

		// our current ACK is one byte larger than the 802.15.4-ACK, so this 
		// has to be at least one byte "longer" than 802.15.4 minimum
		// 802.15.4: (aUnitBackoffPeriod + ) aTurnaroundTime + phySHRDuration + 6 * phySymbolsPerOctet
		//     where aUnitBackoffPeriod = 20 symbols, aTurnaroundTime = 12 symbols, phySHRDuration = 10 symbols for 2.4GHz, 
		//     6 * phySymbolsPerOctet (includes PHR=1 octets and ACK payload=5 octets) = 12 symbols
		// aUnitBackoffPeriod isn't used in the MiXiM 802.15.4 MAC (only relevant for beacon-enabled PANs and the CAP) 
        // double macAckWaitDuration @unit(s) = default(0.000592 s);  // 802.15.4: 0.00056s  1+12+10+12 symbols
        double macAckWaitDuration @unit(s);  // need increase of timings
               
        int ackLength @unit(bit); // 802.15.4: 40 bit 2FrameControl 1seq 2FCS 
        
        double sifs @unit(s); // 12 symbols
        
        double aTurnaroundTime @unit(s); 
              
              
        int macMaxFrameRetries;      
        int macMaxCSMABackoffs;
        int macMinBE;
        int macMaxBE;
		
	
	gates:
        input lowerControlIn;
        output lowerControlOut;
        input lowerGateIn;
        output lowerGateOut;
        input gateReqIn;      
        output gateIndOut;
        
        output gateSnoopIndOut @loose;
}
