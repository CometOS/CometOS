package cometos.src.communication.ipv6.routing;

simple RPL
{
    parameters:
        //DIO - CONFIG
		bool authentificationEnable = default(false);
        
        // keep consistent with RPLRouting.h
		int defaultDioIntervalMin = default(7);
		int defaultDioIntervalDoublings = default(20);
   		int defaultDioRedundancyConst = default(10);
   		int defaultDioMinHopRankIncrease = default(256);
   		
   		bool defaultDioRoot;
   		bool grounded = default(false);
		
        @class(cometos_v6::RPLRouting);
        @display("i=block/routing");
        
       
}
