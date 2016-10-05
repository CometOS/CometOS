package cometos.src.communication.tcp;
import cometos.src.communication.base.LowerEndpoint;

simple TcpComm extends LowerEndpoint
{
    parameters:
        @class(TcpComm);
        // if valid value is given, server is started by instance
 		int listen=default(-1);
 		
 		// you can pass a list (IP:PORT), an node will tries to build up TCP/IP connection to target
 		// "localhost:22300 123.45.21.3:9999"
 		string conn=default("");
 		
	 
}




