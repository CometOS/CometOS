package cometos.src.communication.routing;
import cometos.src.communication.base.Layer;

simple AODV extends Layer
{
    parameters:
        @class(cometos::AODV);
        @display("i=block/routing");
        int sendingOffset @unit(ms)=default(100ms);
      gates:
              
        input rreqIndIn;
        output rreqReqOut;
     
        input rrepIndIn;
        output rrepReqOut;
        
        input rerrIndIn;
        output rerrReqOut;

}
