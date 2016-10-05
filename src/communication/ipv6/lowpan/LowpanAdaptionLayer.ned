package cometos.src.communication.ipv6.lowpan;


import cometos.src.core.Module;

// The LowpanAdaptionLayer compresses the incomming IPv6Request and forwards
// it in Airframes to the MAC Layer. In the other direction it receives 
// Airframes and decompresses the inherited IPv6 Datagrams
simple LowpanAdaptionLayer extends Module
{
    parameters:
        @class(cometos_v6::LowpanAdaptionLayer);
        @display("i=block/layer");
        bool meshUnder = default(false);
        bool unlimitedBuffer = default(false);
        int fixedAvgPktDuration = default(6);
        int macRetryControlMode = default(2);
        int delayMode = default(3);
        
        // the following two parameters specify the minimum delay in multiples
        // of the avgPacketDuration the lowpan layer will wait after each
        // transmission; to the minimum value, a random number drawn from
        // [0, avgPacketDuration) is added; the default value 3/2 therefore
        // causes the lowpan layer to  wait between 1.5 and 2.5 transmission
        // durations after each frame; must fit into a uint8_t!!!
        int minDelayNominator = default(3);
        int minDelayDenominator = default(2);
        //int mEntries = default(28);
        int bufferSize = default(2000);
        int numBufferHandlers;
        int numDirectDatagramHandlers;
        int numReassemblyHandlers;
        
        // 0: FIFO_QUEUE  1: DG_ORDERED_QUEUE
        int queueType = default(0);
        bool enableLFFR = default(false);
        bool enableDirectFwd = default(false);
        int congestionControlType = default(0);
        int timeoutMs = default(2000);
        int queueSwitchAfter = default(3);
        bool pushBackStaleObjects = default(false);
        int queueSize;
        bool useRateTimerForQueueSwitch = default(false);
        
    gates:
        input fromIP;
        output toIP;
        input fromMAC;
        input macSnoop;
        output toMAC;
}
