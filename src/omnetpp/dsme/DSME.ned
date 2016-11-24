package cometos.src.omnetpp.dsme;

//
// IEEE802.15.4e
// Deterministic and synchronous multi-channel extension
//
simple DSME extends cometos.src.communication.ieee802154.mac.MacAbstractionLayer
{
    parameters:
        bool optimizations = default(false);
                                          
        bool isPANCoordinator = default(false);
        bool isCoordinator = default(false);
        double isCoordinatorProbability = default(0.5);
                
        // aMaxPHYPacketSize = 127 Octets (802.15.4-2006, page 45)
        // aMinMPDUOverhead = 9 Octets (802.15.4-2006, page 159)
        // aMaxMACPayloadSize = aMaxPHYPacketSize - aMinMPDUOverhead (802.15.4-2006, page 159)
        int mtu @unit("B") = 127 Byte - 9 Byte;

        int numCSMASlots = 8;

        int superframeOrder = default(3);			// 4 bits
        int multiSuperframeOrder = default(5);		// 3 bits
        int beaconOrder = default(7);				// 4 bits

        int finalCAPSlot = default(8);				// 4 bits

        // Maximum number GTS which a Device is allowed to allocate per destination
        // e.g. NumberOfGTSPerMultisuperframe / NumberOfNeighbors
        int maxNumberGTSAllocPerDevice = default(7);
        int maxNumberGTSAllocPerRequest = default(1); // also per superframe TODO ??
        int maxGTSIdleCount = default(0);	// 0 => never deallocate idle GTS
        
        int macDSMEGTSExpirationTime = default(7);

        string allocationScheme = default("next");	// ("random", "next")

        // Slotted CSMA-CA
        int contentionWindow = default(2);

        int numChannels = default(16);
        int commonChannel = default(11);
        int macPANId = default(10);
        
        string radioModule = default("^.radio");   // The path to the Radio module  //FIXME remove default value
        
       
        // bit rate
        bitrate = default(250000 bps);
        
        // minimum backoff exponent
        // Minimum backoff exponent
        // 802.15.4-2006, page 164
        macMinBE = default(3);

        // Maximum backoff exponent
        // 802.15.4-2006, page 163
        macMaxBE = default(5);
        
        // Maximum number of extra backoffs (excluding the first unconditional one) before frame drop
        // 802.15.4-2006, page 163
        macMaxCSMABackoffs = default(4);
        
        // Maximum number of frame retransmission
        // 802.15.4-2006, page 164
        macMaxFrameRetries = default(3);
        
        @signal[unicastDataSentDown](type=cPacket);
        @signal[broadcastDataSentDown](type=cPacket);
        @signal[commandSentDown](type=cPacket);
        @signal[beaconSentDown](type=cPacket);
        @signal[ackSentDown](type=cPacket);
        
        @statistic[unicastDataSentDown](title="unicast packet sent down of type DATA"; source=unicastDataSentDown; record=count,vector(packetBytes); interpolationmode=none);
        @statistic[broadDataSentDown](title="broadcast packet sent down of type DATA"; source=broadcastDataSentDown; record=count,vector(packetBytes); interpolationmode=none);
        
        @class(::dsme::DSMEPlatform);
}

