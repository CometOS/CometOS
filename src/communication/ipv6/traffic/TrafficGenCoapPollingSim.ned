package cometos.src.communication.ipv6.traffic;


simple TrafficGenCoapPollingSim
{
    parameters:
        @class(cometos_v6::TrafficGenCoapPollingSim);
        xml targetsFile = default(xml("<root/>"));
        int numCollections = default(0);
        int timeoutInterval = default(3000);
        bool isMaster = default(false);
        @display("i=block/app");
}
