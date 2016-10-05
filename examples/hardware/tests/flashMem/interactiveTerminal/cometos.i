%include "cometos.core.i"


%{
#include "SerialComm.h"
#include "SerialFwdBounce.h"
#include "TcpAgent.h"
#include "TcpComm.h"
#include "AsyncTcpAgent.h"
#include "AsyncTcpComm.h"
#include "CommAssessment.h"
#include "SimpleRouting.h"
#include "SystemMonitor.h"
#include "Sniffer.h"
#include "SimpleReliabilityLayer.h"
#include "PeriodicTraffic.h"
#include "PrintfAppReceiver.h"
#include "TimeSyncService.h"
#include "TrickleModule.h"
#include "TopologyMonitor.h"
#include "Statistics.h"
#include "S25FlAccessMsg.h"
#include "palFirmware.h"
%}

%include "S25FlAccessMsg.h"
%include "SystemMonitor.h"
%include "NwkHeader.h"
%include "TcpAgent.h"
%include "TcpComm.h"
%include "AsyncTcpAgent.h"
%include "AsyncTcpComm.h"

%include "SimpleRouting.h"
%include "CommAssessment.h"
%include "RemoteEvent.h"
%include "SerialComm.h"


%ignore cometos::TimeSyncData;
%include "TrickleModule.h"
%include "TimeSyncService.h"


%include "SerialFwdBounce.h"
 
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

%inline %{
uint8_t deref(uint8_t * n) {
   return *n;
}
%}


%include "SList.h"
%template(TrafficData) StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD>;

%template(FlashVector) Vector<uint8_t, FLASH_DATA_MAX_SIZE>;
%extend Vector<uint8_t, FLASH_DATA_MAX_SIZE> {
  uint8_t get(uint8_t idx) {
  	  return self->operator[](idx);	
  }
};


%include "SimpleReliabilityLayer.h"

%template(Sums32) Sums<mac_dbm_t, uint16_t, int32_t, uint32_t>;
%template(SumsMinMaxRssi32) SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t>;
%template(TopologyMonitorX) cometos::TopologyMonitor<TM_LIST_SIZE>;
%template(NodeListX) StaticSList<node_t, TM_LIST_SIZE>;
%extend SerialComm {
	%template(SerialComm) SerialComm<const char*>;
}

%include "PrintfAppReceiver.h"


%include "cometos.otap.i"


	

	