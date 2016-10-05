package cometos.src.communication.ieee802154.mac;
import cometos.src.communication.base.LowerEndpoint;

simple NetworkInterfaceSwitch extends LowerEndpoint
{
    parameters:
        @class(NetworkInterfaceSwitch);
        @display("i=block/layer");
        int cniAddrBegin = default(1);
        int cniAddrEnd = default(1000);

    gates:
        output cniReqOut;
        input cniIndIn;
        output nniReqOut;
        input nniIndIn;
}



