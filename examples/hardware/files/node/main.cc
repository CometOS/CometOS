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

/**This is a full working example for a wireless node. The CometOS
 * default communication stack is included, which supports routing.
 *
 * The example also shows how to use RMI for writing user modules.
 *
 * See TODO for running compatible base station.
 *
 * @author Stefan Unterschuetz
 */
/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "protocolstack.h"
#include "RemoteModule.h"
#include "palLed.h"
#include "Task.h"
#include "OutputStream.h"
#include "FileManager.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

// TODO Write arbiter test

class MyModule: public RemoteModule {
public:
	MyModule(const char* name) :
			RemoteModule(name) {
	}

	void initialize() {
		RemoteModule::initialize();
		remoteDeclare(&MyModule::add, "add");
	}

	uint8_t add(uint8_t &a, uint8_t &b) {
		return a + b;
	}
};

#if 0
// some status led
void toggleLed();
SimpleTask task(toggleLed);
void toggleLed() {
	static bool started = false;
	palLed_toggle(1);
	cometos::getScheduler().add(task, 2000);
	if (!started) {
		started = true;
	}
}
#endif

/* This module need not to be connected to stack,
 * it uses RMI, which are inherently available, if
 * deriving from RemoteModule. RemoteModule uses the
 * singleton class RemoteAccess, which is part of the
 * communication stack
 */
static MyModule module("mod");

static FileManager fm;

int main() {
	palLed_init();

    protocolstack_nwkConnect(fm.getGateIndIn(),fm.getGateReqOut());
	protocolstack_init();
	cometos::initialize();

	getCout() << "Booting" << endl;

	//cometos::getScheduler().add(task, 2000);
	cometos::run();
	return 0;
}

