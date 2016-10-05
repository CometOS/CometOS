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

/**
 * @author Stefan Unterschütz
 */
#include "RemoteAccess.h"
#include "AirString.h"
#include "palLed.h"
#include "logging.h"

// used sending offset if request was broadcast
// this is used to avoid congestion
#define RA_BROADCAST_OFFSET 500

Define_Module(cometos::RemoteAccess);

namespace cometos {

const char * const RemoteAccess::DEFAULT_MODULE_NAME = "ra";

RemoteAccess::RemoteAccess(const char* service_name) :
		Endpoint(service_name) {
}

void RemoteAccess::raise(Airframe* frame, node_t dst) {
#ifdef OMNETPP
	Enter_Method_Silent();
#endif

	// mark frame as event
	(*frame) << (uint8_t) (RA_REMOTE_EVENT);
	sendRequest(new DataRequest(dst, frame));

}

void RemoteAccess::handleIndication(DataIndication* msg) {
	uint8_t type;
	AirString module;
	AirString var;
	uint8_t seq;
	uint16_t counter;

	node_t dst = msg->src;

	uint16_t offset = 0;
	if (msg->dst == BROADCAST) {
		offset = intrand(RA_BROADCAST_OFFSET);
	}

	LOG_INFO("AirframeLen=" << (int) msg->getAirframe().getLength() << "|dst=" << msg->dst << "|src=" << dst);

	msg->getAirframe() >> seq;

	if (seq == RA_REMOTE_EVENT) {
	    LOG_DEBUG("Drop remote event");
	    delete msg;
	    return;
	}

//	LOG_ERROR("seq=" << (int) seq << "|AirframeLen=" << (int) msg->getAirframe().getLength());
	msg->getAirframe() >> module;
//	LOG_ERROR("module=" << module.getStr() << "|AirframeLen=" << (int) msg->getAirframe().getLength());
	msg->getAirframe() >> type;
//	LOG_ERROR("type=" << (int) type << "|AirframeLen=" << (int) msg->getAirframe().getLength());
	msg->getAirframe() >> var;
//	LOG_ERROR("var=" << var.getStr() << "|AirframeLen=" << (int) msg->getAirframe().getLength());

	RemoteModule* mod = (RemoteModule*) Module::getModule(module.getStr());

	if (mod == NULL) {
		Airframe* resp = new Airframe;
		(*resp) << (uint8_t) (RA_ERROR_NO_SUCH_MODULE) << seq; // error flag
		sendRequest(new DataRequest(dst, resp));
		delete msg;
		return;
	}

	if (type == RA_REMOTE_VARIABLE) { // remote variable
		Airframe *resp = new Airframe;
		if (msg->getAirframe().getLength() == 0) {
			// reading failed
			if (!mod->remoteReadVariable(resp, var.getStr())) {
				(*resp) << (uint8_t) (RA_ERROR_NO_SUCH_VARIABLE);
			} else {
				(*resp) << (uint8_t) (RA_SUCCESS);
			}
		} else {
			if (!mod->remoteWriteVariable(&msg->getAirframe(), var.getStr())) {
				(*resp) << (uint8_t) (RA_ERROR_NO_SUCH_VARIABLE);
			} else {
				(*resp) << (uint8_t) (RA_SUCCESS);
			}
		}
		(*resp) << seq;
		sendRequest(new DataRequest(dst, resp),offset);

	} else if (type == RA_REMOTE_METHOD) { // remote method invocation

		Airframe *resp = mod->remoteMethodInvocation(msg->decapsulateAirframe(),
				var.getStr());
#ifdef OMNETPP
		Enter_Method_Silent();
#endif
		if (resp == NULL) {
			resp = new Airframe;
			(*resp) << (uint8_t) (RA_ERROR_NO_SUCH_METHOD); // error flag
		} else {
			(*resp) << (uint8_t) (RA_SUCCESS); // succeed flag
		}
		(*resp) << seq;
		sendRequest(new DataRequest(dst, resp),offset);
	} else if (type == RA_REMOTE_EVENT_SUBSCRIBE) { // remote event subscribe
		Airframe *resp = new Airframe;
		msg->getAirframe() >> counter;
		if (mod->eventSubscribe(var.getStr(), this, dst, counter)) {
			(*resp) << (uint8_t) (RA_SUCCESS); // succeed flag
		} else {
			(*resp) << (uint8_t) (RA_ERROR_NO_SUCH_EVENT); // error flag
		}
		(*resp) << seq;
		sendRequest(new DataRequest(dst, resp),offset);
	} else if (type == RA_REMOTE_EVENT_UNSUBSCRIBE) { // remote event unsubscribe
		Airframe *resp = new Airframe;
		if (mod->eventUnsubscribe(var.getStr(), BROADCAST)) {
			(*resp) << (uint8_t) (RA_SUCCESS); // succeed flag
		} else {
			(*resp) << (uint8_t) (RA_ERROR_NO_SUCH_EVENT); // error flag
		}
		(*resp) << seq;
		sendRequest(new DataRequest(dst, resp),offset);
	} else if (type == RA_REMOTE_METHOD_EVENT){

	    // remote method call with event callback - check method availability,
	    // defer event from remoteMethod (and subscribe) and send back
	    // response -- the called module is responsible for raising the event
	    Airframe * resp = NULL;
	    uint8_t status = RA_SUCCESS;
	    RemoteMethod * m = mod->remoteFindMethod(var.getStr());
	    if (m != NULL) {
	        if (!mod->eventSubscribe(m->evName, this, dst, 1)) {
	            status = RA_ERROR_INVALID_METHOD_TYPE;
	        }
	    } else {
	        status = RA_ERROR_NO_SUCH_METHOD;
	    }

	    if (status == RA_SUCCESS) {
	        resp = mod->remoteMethodInvocation(msg->decapsulateAirframe(),
	                var.getStr());
            #ifdef OMNETPP
                    Enter_Method_Silent();
            #endif
            if (resp == NULL) {
                status = RA_ERROR_NO_SUCH_METHOD;
            }
	    }

	    if (status != RA_SUCCESS) {
	        resp = new Airframe();
	        if (status != RA_ERROR_INVALID_METHOD_TYPE) {
	            mod->eventUnsubscribe(m->evName, dst);
	        }
	    }
	    (*resp) << status;
	    (*resp) << seq;
        sendRequest(new DataRequest(dst, resp),offset);

	} else {
		ASSERT(false);
	}

	delete msg;
}


void RemoteAccess::initialize() {
//	schedule(new cometos::Message, &RemoteAccess::timer1, 1000);

}

//void RemoteAccess::timer1(cometos::Message *msg) {
//	Airframe *air = new Airframe;
//	(*air) << (uint8_t) 4;
//
//	send(new DataRequest(dst, air));
//
//	schedule(msg, &RemoteAccess::timer1, 1000);
//
//}

} /* namespace cometos */
