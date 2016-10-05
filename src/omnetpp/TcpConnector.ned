package cometos.src.omnetpp;
import cometos.src.communication.base.LowerEndpoint;


simple TcpConnector extends LowerEndpoint
{
    parameters:
    @class(TcpConnector);
    int port = default(0);
    @display("i=block/rxtx");
}
