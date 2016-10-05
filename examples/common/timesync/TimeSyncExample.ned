package cometos.examples.common.timesync;

simple TimeSyncExample
{
    parameters:
        @class(TimeSyncExample);
        int timeOffset @unit(ms) = default(0 ms);
}

