package cometos.src.communication.ipv6.traffic;


simple TrafficGenSim extends TrafficGen
{
    parameters:
        @class(TrafficGenSim);
        xml routingFile = default(xml("<root/>"));
        int startWith = default(0);
        int endWith = default(250);
        int totalData = default(60000);
        @display("i=block/app");
}
