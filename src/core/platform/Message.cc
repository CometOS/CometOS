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

#include "Message.h"
#include "OutputStream.h"
#include "palExec.h"
using namespace std;

/*METHOD DEFINITION----------------------------------------------------------*/

#ifdef ENABLE_LOGGING
#include "Module.h"
#include "logging.h"

/**In case of enabled logging a task may define his log level,
 * this log level can be used to filter log information during execution.*/
uint8_t cometos::Message::getLogLevel() {
//	cometos::Module* it =  cometos::Module::first;
//	while (it != NULL) {
//		if (it==owner) {
//			return it->log_level;
//		}
//		it = it->next;
//	}
    if (owner != NULL) {
        cometos::Module * mod = (cometos::Module *) owner;
        return mod->log_level;
    }
	return LOG_LEVEL_ERROR;
}

#endif

const cometos::Module* cometos::Message::getContext() const {
    return owner;
}


#ifdef DEBUG_MESSAGE_ALLOCATION
cometos::Message::MessageList &cometos::Message::messageList() {
	static MessageList list;
	return list;
}

void cometos::Message::printOwners() {
    getCout() << "|";
	for (Message::MessageList::iterator it =
			Message::messageList().begin(); it
			!= Message::messageList().end(); it++) {

		Module* mod = (Module*) (*it)->owner;
		if (mod != NULL && mod->getName() != NULL) {
			getCout() << mod->getName() << "|";
		} else {
			getCout() << ".";
		}

	}
	getCout() << cometos::endl;
}

void cometos::Message::printAllocationCount() {
	getCout() << (int) messageList().size();
}

#endif

cometos::Message::~Message() {
#ifdef DEBUG_MESSAGE_ALLOCATION
	palExec_atomicBegin();
	MessageList::iterator it = messageList().find(this);
	messageList().erase(it);
	palExec_atomicEnd();
#endif
}

cometos::Message::Message() :
			owner(NULL), offset(0xFFFF), inputGate(NULL) {
    event.setCallback(this);
#ifdef DEBUG_MESSAGE_ALLOCATION
	palExec_atomicBegin();
	messageList().push_back(this);
	palExec_atomicEnd();
#endif
}

