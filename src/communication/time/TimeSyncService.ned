package cometos.src.communication.time;

simple TimeSyncService
{
    parameters:
        bool startAtInit = default(true);
        bool initiallyIsMaster = default(false);
        @class(cometos::TimeSyncService);

    gates:
        input gateTimestampIn;
        output gateTimestampOut;
        input gateInitialIn;
        output gateInitialOut;
}
