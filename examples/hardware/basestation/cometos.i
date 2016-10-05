%include "cometos.core.i"

%ignore cometos::SystemMonitor::heartbeat;
%ignore cometos::SystemMonitor::asyncTest;

%{
#include "SerialComm.h"
#include "FwdBounce.h"
#include "TcpAgent.h"
#include "TcpComm.h"
#include "AsyncTcpAgent.h"
#include "AsyncTcpComm.h"
#include "CommAssessment.h"
#include "SimpleRouting.h"
#include "SystemMonitor.h"
using cometos::SystemMonitor;
#include "Sniffer.h"
#include "SimpleReliabilityLayer.h"
#include "PeriodicTraffic.h"
#include "PrintfAppReceiver.h"
#include "TimeSyncService.h"
#include "TrickleModule.h"
#include "TopologyMonitor.h"
#include "Statistics.h"
#include "PersistableConfig.h"
#include "TxPowerLayer.h"
%}

%typemap(in) timeOffset_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) timeOffset_t = int;

%typemap(out) timeOffset_t {
    $result = PyInt_FromLong($1);
}

%include "SystemMonitor.h"
%include "NwkHeader.h"
%include "TcpAgent.h"
%include "TcpComm.h"
%include "AsyncTcpAgent.h"
%include "AsyncTcpComm.h"

%include "SimpleRouting.h"
%include "CommAssessment.h"
%include "RemoteEvent.h"
%include "PersistableConfig.h"
%include "TxPowerLayer.h"

%include "SerialComm.h"
%template(SerialComm) cometos::SerialComm::SerialComm<const char*>;

%ignore cometos::TimeSyncData;
%include "TrickleModule.h"
%include "TimeSyncService.h"


%include "FwdBounce.h"
 
%feature("director") cometos::Sniffer;
%feature("nodirector") cometos::Sniffer::initialize;
%feature("nodirector") cometos::Sniffer::handleRequest;
%feature("nodirector") cometos::Sniffer::handleIndication;
%include "Sniffer.h"

%include "PeriodicTraffic.h"

%include "Statistics.h"
%include "TopologyMonitor.h"

// to be able to read out reference return values of simple types within python
%inline %{
node_t deref(node_t * n) {
   return *n;
}
%}


%include "SList.h"
%template(TrafficData) cometos::StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD>;

%include "SimpleReliabilityLayer.h"



//%template(SumsTs64) Sums<time_ms_t, uint16_t, uint64_t, uint64_t>;
//%template(SumsRssi32) Sums<mac_dbm_t, uint16_t, int32_t, uint32_t>;
//%template(SumsMinMaxTs64) SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t>;

%template(SumsTs64) Sums<time_ms_t, uint16_t, uint64_t, uint64_t>;
%template(SumsRssi32) Sums<mac_dbm_t, uint16_t, int32_t, uint32_t>;
%template(SumsMinMaxTs64) SumsMinMax<time_ms_t, uint16_t, uint64_t, uint64_t>;
%template(SumsMinMaxRssi32) SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t>;
%template(TopologyMonitorX) cometos::TopologyMonitor<TM_LIST_SIZE>;
%template(NodeListBase) cometos::SListBase<cometos::TMNodeId>;
%template(NodeListX) cometos::StaticSList<cometos::TMNodeId, TM_LIST_SIZE>;



%include "PrintfAppReceiver.h"

%include "cometos.otap.i"


	

	
