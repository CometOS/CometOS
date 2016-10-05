package cometos.src.communication.ipv6.coap;


import cometos.src.core.Module;


simple CoAPTest extends Module
{
	    parameters:
       		@class(CoAPTest);
       		bool sender = default(false);
       		int receiver = default(0);
       		@display("i=block/app");
       		
	
}
