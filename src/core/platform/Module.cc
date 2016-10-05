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

/*INCLUDES-------------------------------------------------------------------*/

#include "Module.h"
#include "string.h"
#include "OutputStream.h"
#ifdef ENABLE_LOGGING
#include "logging.h"
#endif

/*METHOD DEFINITION----------------------------------------------------------*/

namespace cometos {

/*void module_handleMessage(void *m) {
 cometos::Message* msg = static_cast<cometos::Message*> (m);
 BaseInputGate *in = msg->inputGate;
 if (in->receiveMsg(msg)) { // return value indicates whether gate is temp
 delete in;
 }
 }*/

//}
//using namespace cometos;
Module* Module::first = NULL;

void Module::cancel(Message *msg) {
	if (msg) {
	    getScheduler().remove(*msg);
	}

}

int Module::numModules = 0;

Module::Module(const char* service_name)
#ifdef ENABLE_LOGGING
: log_level(LOG_LEVEL_FATAL)
#endif
{
	// build up module list
	next = first;
	first = this;
	numModules++;
	setName(service_name);
}

Module::~Module() {
    if (this == first) {
        first = this->next;
    } else {
        Module* curr = first;
        while(curr->next != NULL) {
            if (curr->next == this) {
                curr->next = this->next;
                break; // we found the module, remove and leave loop
            }
            curr = curr->next;
        }
    }
    numModules--;
}

void Module::setName(const char* service_name) {
	if (service_name) {
		strncpy(name, service_name, MODULE_NAME_LENGTH - 1);
		name[MODULE_NAME_LENGTH - 1] = 0;
	} else {
		name[0] = 0;
	}
}

bool Module::isScheduled(Message * msg) {
	if (msg) {
		return msg->isScheduled();
	} else {
		return false;
	}
}

void Module::initializeAll() {
	Module* it = first;
	while (it != NULL) {
		it->initialize();
		it = it->next;
	}
}


Module* Module::getModule(const char* name, uint8_t idx) const {

	Module* it = first;
//#if defined BOARD_devboard or defined BOARD_deRF2RCB
//#ifdef SERIAL_PRINTF
//	char tmp[MODULE_NAME_LENGTH] = {it->name[0], it->name[1], it->name[2], it->name[3], 0};
//
//	getCout() << tmp << "|" << (uintptr_t) it<< "\n";
//#endif
//#endif
	int i = 0;
	while (it != NULL) {
		if (it->name != NULL && 0 == strcmp(it->name, name)) {
//#if defined BOARD_devboard or defined BOARD_deRF2RCB
//#ifdef SERIAL_PRINTF
//		    getCout() << "!\n";
//#endif
//#endif
			return it;
		}
		i++;
		if (i > numModules) {
		    ASSERT(false);
		}
		it = it->next;
//#if defined BOARD_devboard or defined BOARD_deRF2RCB
//#ifdef SERIAL_PRINTF
//		tmp[0] = it->name[0]; tmp[1] = it->name[1]; tmp[2] = it->name[2]; tmp[3]= it->name[3]; tmp[4] = 0;
//		getCout() << tmp << "|" << (uintptr_t) it<< "\n";
//#endif
//#endif
	}
	return NULL;
}
} // namespace cometos
