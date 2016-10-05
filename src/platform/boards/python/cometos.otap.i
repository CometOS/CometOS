%{
#include "OtapBlockTransfer.h"
#include "OtapBase.h"
#include "OtapTaskDone.h"
#include "palFirmware.h"
%}

%ignore RunMessage;

%typemap(in) palFirmware_slotNum_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) palFirmware_slotNum_t = int;

%typemap(out) palFirmware_slotNum_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) palFirmware_segNum_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) palFirmware_segNum_t = int;

%typemap(out) palFirmware_segNum_t {
    $result = PyInt_FromLong($1);
}

%typemap(in) palFirmware_crc_t {
    $1 = PyInt_AsLong($input);
}
%typemap(typecheck) palFirmware_crc_t = int;

%typemap(out) palFirmware_crc_t {
    $result = PyInt_FromLong($1);
}


%include "OtapBlockTransfer.h"
%feature("director")  cometos::OtapBlockTransfer<4, 3, 64>;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::initialize;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::handleIndication;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::recvSegment;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::getNumPkts;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::getRedPkts;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::getPktSize;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::tryDecoding;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::encode;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::clearRxHist;
%feature("nodirector") cometos::OtapBlockTransfer<4, 3, 64>::setRxHist;
%template(PyOtapBlockTransfer) cometos::OtapBlockTransfer<4, 3, 64>;
%include "OtapBase.h"

%include "OtapTaskDone.h"

%include "BitVector.h"
%template(SegBitVector) cometos::BitVector<OTAP_BITVECTOR_SIZE>; 
