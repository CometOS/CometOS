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
#include "MinAddressingBase.h"
#include "OutputStream.h"
/*METHOD DEFINITION----------------------------------------------------------*/

namespace cometos {

using namespace omnetpp;

void Module::cancel(Message* msg) {
	ASSERT(msg!=NULL);

	// cancel scheduled message
	// if message contains allocated InputGate, then this is freed
	if (msg->inputGate && msg->inputGate->anonymous) {
		delete msg->inputGate;
	}
	msg->inputGate = NULL;
	// remove message from OMNeT++ scheduler
	cancelEvent(msg);
}

Module::Module(const char* service_name) {
}



Module* Module::getModule(const char* name, uint8_t idx) {
	if (idx != 0xff) {
		return check_and_cast<Module*>(
				getParentModule()->getSubmodule(name, idx));
	} else {
		cModule *mod = getParentModule()->getSubmodule(name);
		if (mod != NULL) {
		    return check_and_cast<Module*>(mod);
		} else {
		    return NULL;
		}
	}
}

const char* Module::getName() const {
	return cModule::getName();
}


Module::~Module() {
}

timestamp_t Module::getTime() {
	return simTime().dbl() * 1000;
}

node_t Module::getId() const{
    cModule * tmp = getParentModule()->getSubmodule(ADDRESSING_MODULE_NAME);
    if (tmp!=NULL) {
        MinAddressingBase * addr = check_and_cast<MinAddressingBase *>(tmp);
        return addr->getShortAddr();
    } else {
        return (int) getParentModule()->par("id");
    }
}

void Module::initialize() {

}

void Module::initializeGates() {

	cGate* g;

	for (std::list<BaseInoutGate*>::iterator it = inoutGates.begin();
			it != inoutGates.end(); it++) {
		gateConverter[gateHalf((*it)->name, cGate::INPUT)] = *it;
		(*it)->out = gateHalf((*it)->name, cGate::OUTPUT);
	}

	for (std::list<BaseInputGate*>::iterator it = inputGates.begin();
			it != inputGates.end(); it++) {
		g = this->gate((*it)->name);
		gateConverter[g] = *it;
	}

	for (std::list<BaseOutputGate*>::iterator it = outputGates.begin();
			it != outputGates.end(); it++) {
		g = this->gate((*it)->name);
		(*it)->out = g;
	}

	for (std::list<BaseInputGateArray*>::iterator it = inputGateArrays.begin();
			it != inputGateArrays.end(); it++) {
		for (uint8_t i = 0; i < (*it)->getLength(); i++) {
			BaseInputGate *base = (*it)->getBaseGate(i);
			g = this->gate(base->name, i);
			gateConverter[g] = base;
		}

	}

	for (std::list<BaseOutputGateArray*>::iterator it = outputGateArrays.begin();
			it != outputGateArrays.end(); it++) {
		for (uint8_t i = 0; i < (*it)->getLength(); i++) {
			BaseOutputGate *base = (*it)->getBaseGate(i);
			g = this->gate(base->name, i);
			base->out = g;
		}
	}

}

Object* Module::command(const char *argv[], const uint8_t argc) {
	if (argc > 0) {
//		printf("unknown command %s!\n", argv[0]);
	} else {
//		printf("no command!");
	}
	return NULL;
}

bool Module::isScheduled(Message *msg) {
	if (msg == NULL) {
		return false;
	}
	return msg->isScheduled();
}

void Module::check(Message *msg) {
	//ASSERT(msg->inputGate==NULL);
	//ASSERT(msg->msgOwner==NULL);

}

void Module::handleMessage(cMessage *msg) {

	Message *m = check_and_cast<Message*>(msg);
	ASSERT(m!=NULL);

	BaseInputGate *handler = m->inputGate;

	if (m->owner != NULL) {
		ASSERT(msg->isSelfMessage());
		ASSERT( m->owner==this);
		m->delegate_(this, m);
		return;
	}

	if (msg->isSelfMessage()) {
		ASSERT(handler!=NULL);
		m->inputGate = NULL;
		handler->receiveMsg(m);
		ASSERT(handler->anonymous);
		delete handler;
		return;
	}


	ASSERT(gateConverter.count(m->getArrivalGate())>0);

	m->inputGate = gateConverter[m->getArrivalGate()]; // TODO check why this is not already done
	handler = m->inputGate;
	ASSERT(handler!=NULL);

	handler->receiveMsg((Message*) msg);

	ASSERT(handler->anonymous==false);
	/*if (handler->anonymous) {
	 ASSERT(handler!=NULL);
	 delete handler;
	 }*/
}

void Module::schedule_dbl(Message *msg, BaseInputGate* gate, double s) {

	msg->inputGate = gate;
	cSimpleModule::scheduleAt(simTime() + s, msg);

}

} // namespace
