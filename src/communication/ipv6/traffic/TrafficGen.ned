package cometos.src.communication.ipv6.traffic;


import cometos.src.core.Module;


simple TrafficGen extends Module
{
    parameters:
        @class(TrafficGen);
        int offset = default(0);
        int payloadSize = default(0);
        bool mRTT = default(false);
        int rttDest = default(0);
        int maxRuns = default(1);
        int ratePer256s = default(10);
        bool mTP = default(false);
        int tpDest = default(0);
        int mode = default(1);
        int splitDatagramInto = default(1);
        @display("i=block/app");
}
