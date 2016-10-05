// directors enable cross polymorphism
%module(directors="1") cometos
%{

#define SWIG_FILE_WITH_INIT
#include "primitives.h"
#include "AirString.h"
#include "Airframe.h"
#include "NwkHeader.h"
#include "Module.h"
#include "Gate.h"
#include "Object.h"
#include "Message.h"
#include "Gate.h"
#include "Endpoint.h"
#include "Layer.h"
#include "LowerEndpoint.h"
#include "EndpointWrap.h"
#include "LowerEndpointWrap.h"
#include "RequestResponse.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "Dispatcher.h"
#include "MinAddressingBase.h"
#include "ModuleWrap.h"
#include "cometos.h"
#include "RemoteModule.h"
#include "RemoteEvent.h"
#include "RemoteAccess.h"
#include "palExec.h"
#include "BitVector.h"
#include "helper.h"
#include "palLocalTime.h"
#include "stringparser.h"
#include "mac_interface.h"
#include "cometosError.h"
#include "NetworkTime.h"
#include "Callback.h"
%}

%typemap(in) uint8_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) uint8_t = int;


%typemap(out) uint8_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) uint16_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) uint16_t = int;

%typemap(out) uint16_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) int32_t {
    $1 = PyInt_AsLong($input);
}

%typemap(typecheck) int32_t = long;

   
%typemap(out) int32_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) uint32_t {
    $1 = PyInt_AsLong($input);
}

%typemap(typecheck) uint32_t = long;

   
%typemap(out) uint32_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) time_ms_t {
    $1 = PyInt_AsLong($input);
}

%typemap(typecheck) time_ms_t = long;

   
%typemap(out) time_ms_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) uint64_t {
    $1 = PyLong_AsLong($input);
}
%typemap(typecheck) uint64_t = long;

%typemap(out) uint64_t {
    $result = PyLong_FromLong($1);
}

%typemap(in) node_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) node_t = int;


%typemap(out) node_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) firmwareVersion_t {
    $1 = PyLong_AsLong($input);
}
%typemap(typecheck) firmwareVersion_t = long;


%typemap(out) firmwareVersion_t {
    $result = PyLong_FromLong($1);
}


%typemap(in) cometos_error_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) cometos_error_t = int;

%typemap(out) cometos_error_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) mac_ccaMode_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_ccaMode_t = int;

%typemap(out) mac_ccaMode_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) mac_dbm_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_dbm_t = int;

%typemap(out) mac_dbm_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) mac_networkId_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_networkId_t = int;

%typemap(out) mac_networkId_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) mac_channel_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_channel_t = int;

%typemap(out) mac_channel_t {
    $result = PyInt_FromLong($1);
}


%typemap(in) mac_txMode_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_txMode_t = int;

%typemap(out) mac_txMode_t {
    $result = PyInt_FromLong($1);
}


%typemap(in) mac_power_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) mac_power_t = int;

%typemap(out) mac_power_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) networkTimestamp_t {
    $1 = PyLong_AsLong($input);
}
%typemap(typecheck) networkTimestamp_t = long;

%typemap(out) networkTimestamp_t {
    $result = PyLong_FromLong($1);
}

%{
// classes used via reference by parsed classes, but which are NOT parsed themselves
// we need to give a using declaration, because otherwise the namespace is not 
// deduced correctly
using cometos::Callback;
using cometos::TaskScheduler;
using cometos::Event;
using cometos::OutputStream;
using cometos::RemoteMethod;
using cometos::BitVector;
using cometos::RemoteAccess;
using cometos::RemoteEvent;
using cometos::StaticSList;
%}

// WRAP CORE ------------------------------------------------------------------
// create dummy wrapper for classes we don't want to wrap
class Platform{};

%include "Task.h"
//class ObjectContainer{};
%include "Module.h"
%include "Gate.h"
%ignore cometos::Message::operator=;
%include "Message.h"
%include "cometos.h"
%include "Object.h"
%include "cometosError.h"
// also wrap palLocalTime to access the common time base from python scripts
%include "palLocalTime.h"
%include "NetworkTime.h"
// WRAP COMM ------------------------------------------------------------------

// enalbe cross polymorphism for all classes
%feature("director") EndpointWrap;   
%feature("director") LowerEndpointWrap;   
%feature("director") ModuleWrap;   
%feature("director") SerialFwdBounce;
%feature("nodirector") SerialFwdBounce::initialize;  
%feature("nodirector") SerialFwdBounce::handleIndication;  

%ignore cometos::Vector::operator=;
%ignore cometos::Vector::operator[];


%ignore cometos::BaseVector::operator=;
%ignore cometos::BaseVector::operator[];

// ATTENTION: DO NOT CHANGE ORDER OF INCLUDES FOR Vector.h AND primitives.h,
// THIS LEADS TO PROBLEMS IN OVERLOAD RESOLUTION IN PYTHON (SWIG_AsVal_bool check
// is moved to front and chosen, even if a different type is actually entered
// (nearly everything in python can be interpreted as a boolean value) and therefore,
// the serialization may FAIL!!!!! We additionally fixed this by removing 
// the 'using namespace cometos' directive from all headers within cometos, because
// this directive removed the compilation errors, which will now indicate that 
// something is wrong
%include "Vector.h"
%include "primitives.h"

%template(ByteVector) cometos::BaseVector<uint8_t>; 
%ignore cometos::Airframe::operator=;
%include "Airframe.h"

%include "RequestResponse.h"
%include "DataIndication.h"
%include "DataResponse.h"
%include "DataRequest.h"
%template(RequestWithDataResponse) cometos::Request<cometos::DataResponse>;
%include "helper.h"

%include "RemoteModule.h"

%include "Endpoint.h"
%include "LowerEndpoint.h"
%include "Layer.h"
%include "Dispatcher.h"

%include "ModuleWrap.h"
%include "EndpointWrap.h"
%include "LowerEndpointWrap.h"

%template(DataIndicationOutputGate) cometos::OutputGate<cometos::DataIndication>; 
%template(DataIndicationInputGate) cometos::InputGate<cometos::DataIndication>; 
%template(DataRequestOutputGate) cometos::OutputGate<cometos::DataRequest>; 
%template(DataRequestInputGate) cometos::InputGate<cometos::DataRequest>; 


// following defines are ncessary for working with dispatcher
%template(Dispatcher2) cometos::Dispatcher<2>; 
%template(Dispatcher4) cometos::Dispatcher<4>; 
%template(Dispatcher8) cometos::Dispatcher<8>; 
%template(DataRequestInputGateArray2) cometos::InputGateArray<cometos::DataRequest, 2>;
%template(DataRequestInputGateArray4) cometos::InputGateArray<cometos::DataRequest, 4>;
%template(DataRequestInputGateArray8) cometos::InputGateArray<cometos::DataRequest, 8>;
%template(DataIndicationOutputGateArray2) cometos::OutputGateArray<cometos::DataIndication, 2>;
%template(DataIndicationOutputGateArray4) cometos::OutputGateArray<cometos::DataIndication, 4>;
%template(DataIndicationOutputGateArray8) cometos::OutputGateArray<cometos::DataIndication, 8>;

%include "carrays.i"
%array_class(uint8_t, uint8Array);

%include "AirString.h"


%pythoncode %{

import atexit;
import time;

def addShutdownHook(fun):
	atexit.register(fun)

def stopCometOS():
	print "STOP COMETOS"
	_cometos.stopThread()

def run():
	time.sleep(0.2) # wait some time until all threads are started
	print "RUN COMETOS"
	atexit.register(stopCometOS);
	_cometos.runThread()
	
def initialize():
	print "INITIALIZE COMETOS"
	_cometos.initialize()
%}
