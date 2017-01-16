/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * main.cc
 *
 * Created by the 'createMain.py' script. Please check the namespaces and 
 * instantiation of template classes.
 *
 * created on: 2014-09-22
 *
 */

// Derived from CoAP_Sink_Node

/*INCLUDES-------------------------------------------------------------------*/
 
#include "cometos.h"
 
#include "palLed.h"

#include "CsmaMac.h"
#include "Module.h"

using namespace cometos;

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

class BroadcastData : public Module {
public:
    BroadcastData() :
        Module(),
        count(0),
        dataOut(this, "dataOut"),
        dataIn(this, &BroadcastData::handleIndication, "dataIn")
    {}

    void initialize() {
        schedule(new Message(), &BroadcastData::sendMessage, 1000);
        palLed_toggle(1);
    }

    void sendMessage(Message* msg) {
        schedule(msg, &BroadcastData::sendMessage, 1000);
        palLed_toggle(1);
        AirframePtr frame = make_checked<Airframe>();
        (*frame) << count;
        DataRequest* dr = new DataRequest(0xFFFF, frame);
        dataOut.send(dr);
        count++;
    }

    void handleIndication(DataIndication* msg) {
        delete msg;
    }

    uint16_t    count;

    OutputGate<DataRequest> dataOut;
    InputGate<DataIndication>  dataIn;
};

/*PROTOTYPES-----------------------------------------------------------------*/

static CsmaMac mac("mac");
static BroadcastData sender;


//#include <avr/eeprom.h>
int main() {

/*WIRING---------------------------------------------------------------------*/

    sender.dataOut.connectTo(mac.gateReqIn);

/*INITIALIZATION-------------------------------------------------------------*/


	initialize();
	
/*START----------------------------------------------------------------------*/

	run();
	
	return 0;
}
