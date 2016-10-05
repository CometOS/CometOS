package cometos.src.communication.ipv6.lowpan;

simple LowpanOrderedForwarding like ILowpanCongestionControl
{
    parameters:
        int beMin;
        int beMax;
        int maxNumSentWithoutConfirmation;
        bool tagAllQueueObjects;
        bool useCongestionFlag;
        @class(cometos_v6::LowpanOrderedForwarding);
}
