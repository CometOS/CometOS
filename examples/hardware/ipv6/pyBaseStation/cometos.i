%include "cometos.core.i"
%include "cometos.otap.i"


%include "cpointer.i"

%{
#include "TcpAgent.h"
#include "TcpComm.h"
#include "AsyncTcpAgent.h"
#include "AsyncTcpComm.h"
#include "CommAssessment.h"
#include "SimpleRouting.h"
#include "MacStats.h"
#include "MacControl.h"
#include "PersistableConfig.h"
#include "MacConfig.h"
#include "SerialMac.h"

#include "SystemMonitor.h"
#include "TxPowerLayer.h"
#include "PrintfAppReceiver.h"
#include "RemotelyConfigurableModule.h"

#include "MacAbstractionBase.h"
#include "mac_definitions.h"
#include "NwkStruct.h"
#include "lowpan-macros.h"
#include "FragMetadata.h"
#include "LowpanAdaptionLayer.h"
#include "QueueFragment.h"
#include "DgOrderedLowpanQueue.h"
#include "LowpanOrderedForwarding.h"
#include "LowpanDispatcher.h"
#include "LowpanIndication.h"
#include "LowpanBuffer.h"
#include "lowpanconfig.h"
#include "LowpanQueue.h"
#include "QueueObject.h"
#include "DatagramReassembly.h"
#include "FragmentHandler.h"
#include "AssemblyBuffer.h"
#include "QueuePacket.h"
#include "IPHCCompressor.h"
#include "IPHCDecompressor.h"
#include "Ieee802154MacAddress.h"


#include "FollowingHeader.h"
#include "LowpanAdaptionLayerStructures.h"
#include "IPv6DestinationOptionsHeader.h"
#include "IPv6ExtensionHeaderOption.h"
#include "IPv6FragmentHeader.h"
#include "IPv6HopByHopOptionsHeader.h"
#include "IPv6RoutingHeader.h"

#include "ICMPv6.h"
#include "ICMPv6Message.h"
#include "IpForward.h"
#include "IPv6Address.h"
#include "IPv6Datagram.h"
#include "IPv6InterfaceTable.h"
#include "UnknownProtocol.h"
#include "ContentResponse.h"
#include "ContentRequest.h"
#include "IPv6Response.h"
#include "IPv6Request.h"
#include "NeighborDiscovery.h"
#include "RoutingBase.h"
#include "RoutingTable.h"
#include "StaticRouting.h"
#include "UDPLayer.h"
#include "UDPPacket.h"
#include "TrafficGen.h"
#include "SerialComm.h"
#include "Statistics.h"
#include "TrickleModule.h"
#include "TimeSyncService.h"
#include "SimpleReliabilityLayer.h"
#include "TopologyMonitor.h"
#include "SList.h"
#include "LongScheduleModule.h"
#include "LinkLayerInformation.h"
%}


%typemap(in) timeOffset_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) timeOffset_t = int;

%typemap(out) timeOffset_t {
    $result = PyInt_FromLong($1);
}


%typemap(in) mac_dbm_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_dbm_t = int;

%typemap(out) mac_dbm_t {
    $result = PyInt_FromLong($1);
}


%typemap(in) delayMode_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) delayMode_t = int;

%typemap(out) delayMode_t {
    $result = PyInt_FromLong($1);
}


%typemap(in) time_ms_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) time_ms_t = int;

%typemap(out) time_ms_t {
    $result = PyInt_FromLong($1);
}

%include "MinAddressingBase.h"

%template(ContentRequestOutputGate) cometos::OutputGate<cometos_v6::ContentRequest>;
%template(ContentRequestInputGate) cometos::InputGate<cometos_v6::ContentRequest>;
%template(LowpanIndicationOutputGate) cometos::OutputGate<cometos_v6::LowpanIndication>;
%template(LowpanIndicationInputGate) cometos::InputGate<cometos_v6::LowpanIndication>;
%template(IPv6RequestOutputGate) cometos::OutputGate<cometos_v6::IPv6Request>;
%template(IPv6RequestInputGate) cometos::InputGate<cometos_v6::IPv6Request>;

%include "FragMetadata.h"
%ignore PersistableConfig::FlagMasks;
%ignore cometos::PersistableConfig::FlagMasks;
%ignore FlagMasks;
%include "PersistableConfig.h"

%include "RemotelyConfigurableModule.h"
%template(UdpCfgModule) cometos::RemotelyConfigurableModule<cometos_v6::UdpConfig>;
%template(LowpanCfgModule) cometos::RemotelyConfigurableModule<cometos_v6::LowpanConfig>;
%template(LoofCfgModule) cometos::RemotelyConfigurableModule<cometos_v6::LowpanOrderedForwardingCfg>;
%template(IpCfgModule) cometos::RemotelyConfigurableModule<cometos_v6::IpConfig>;


%ignore IPv6Context::operator=;
%ignore IPv6Address::operator=;
%ignore uint16_nbo::operator=;
%ignore uint32_nbo::operator=;
%include "IPv6Address.h"

//%include "lowpan-macros.h"
%include "FollowingHeader.h"
%include "IPv6Datagram.h"

%ignore cometos_v6::Ieee802154MacAddress::operator=;
%include "Ieee802154MacAddress.h"

%ignore cometos_v6::BufferInformation::operator[];
%include "LowpanBuffer.h"

%include "ContentResponse.h"
%template(RequestWithContentResponse) cometos::Request<cometos_v6::ContentResponse>;
%include "ContentRequest.h"
%include "IPv6Response.h"
%template(RequestWithIPv6Resposne) cometos::Request<cometos_v6::IPv6Response>;
%include "IPv6Request.h"

%include "LinkLayerInformation.h"
%include "QueueObject.h"
%include "LowpanQueue.h"
%include "IPHCDecompressor.h"
%include "IPHCCompressor.h"
%include "DatagramReassembly.h"
%include "FragmentHandler.h"
%include "AssemblyBuffer.h"
%include "QueuePacket.h"
%include "TxPowerLayer.h"

%include "IPv6InterfaceTable.h"
%include "RoutingTable.h"
%include "RoutingBase.h"
%include "StaticRouting.h"
%include "NeighborDiscovery.h"
%include "ICMPv6.h"
%include "UDPLayer.h"
%include "UnknownProtocol.h"
%include "IpForward.h"
%include "lowpanconfig.h"
%ignore cometos_v6::LowpanAdaptionLayerStats::DURATION_FACTOR;
%ignore cometos_v6::LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;
%include "LowpanAdaptionLayerStructures.h"
%include "LowpanIndication.h"
%ignore cometos_v6::QueueFragment::getDirectPacket;
%include "QueueFragment.h"
%include "DgOrderedLowpanQueue.h"
%template(BaseVector2) cometos::BaseVector<uint16_t>;
%template(Vector2_5) cometos::Vector<uint16_t, 5>;
%include "LowpanOrderedForwarding.h"
%include "LowpanAdaptionLayer.h"
%include "LowpanDispatcher.h"

%include "PrintfAppReceiver.h"


%include "SerialComm.h"
%template(SerialComm) cometos::SerialComm::SerialComm<const char*>;

%include "LongScheduleModule.h"
%include "TrickleModule.h"

%include "TimeSyncService.h"
%include "SimpleReliabilityLayer.h"
%include "RemoteEvent.h"
%include "SystemMonitor.h"

// NOTE: To make SWIG recognize the correct template Vector
//       we really have to provide, e.g., 7+1, if the C++ file 
//       (as is currently the case) 
//       uses MAC_DEFAULT_MAX_FRAME_RETRIES+1 in its template
//       instantiation...
%template(BaseVector4) cometos::BaseVector<uint32_t>;
%template(Vector4_8) cometos::Vector<uint32_t, 7+1>;
%include "MacStats.h"
%include "MacConfig.h"
%include "MacControl.h"
%include "SerialMac.h"

%include "Statistics.h"

%pointer_class(node_t, node_tp)
%pointer_class(uint16_t, uint16_tp)
%pointer_class(unsigned short, ushortp)
%include typemaps.i
%apply int &output { node_t& id };
%include "TopologyMonitor.h"



%inline %{
uint16_t deref16(uint16_t * n) {
    return *n;
}

node_t deref(node_t * n) {
   return *n;
}

uint32_t deref(uint32_t * n) {
    return *n;
}
%}


%ignore SList::operator[];
%include "SList.h"

// have to specify base class template specs to. could also define 
// corresponding methods in subclass
%template(SumsTs64) Sums<time_ms_t, uint16_t, uint64_t, uint64_t>;
%template(SumsRssi32) Sums<mac_dbm_t, uint16_t, int32_t, uint32_t>;
%template(SumsMinMaxTs64) SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t>;
%template(SumsMinMaxRssi32) SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t>;
%template(TopologyMonitorX) cometos::TopologyMonitor<TM_LIST_SIZE>;
%template(NodeListBase) cometos::SListBase<TMNodeId>;
%template(NodeListX) cometos::StaticSList<TMNodeId, TM_LIST_SIZE>;



%template(LowpanDispatcher2) cometos_v6::LowpanDispatcher<2>; 

// declare which methods should be handled as polymorph by swig
%feature("director") TrafficGen;
%feature("nodirector") TrafficGen::~TrafficGen;
%feature("nodirector") TrafficGen::initialize;
%feature("nodirector") TrafficGen::ICMPListener;
%feature("nodirector") TrafficGen::finish;
%feature("nodirector") TrafficGen::offsetTimerFired;
%include "TrafficGen.h"

%ignore operator==;


// WRAP MODULES ---------------------------------------------------------------

%include "NwkHeader.h"

%include "TcpAgent.h"
%include "AsyncTcpAgent.h"
%include "TcpComm.h"
%include "AsyncTcpComm.h"

%include "SimpleRouting.h"
%include "CommAssessment.h"
