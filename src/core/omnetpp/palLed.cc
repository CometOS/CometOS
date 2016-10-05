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

#include "palLed.h"
#include <omnetpp.h>
#include "simUtil.h"
#include <map>

using namespace omnetpp;

enum LedCmd {
    LED_ON,
    LED_OFF,
    LED_TOGGLE
};

static std::map<int, int> ledState;

#define modifyDisplay(mod,type,index,value) 	if ((*cSimulation::getActiveEnvir()).isGUI()) \
		mod->getParentModule()->getDisplayString().setTagArg(type,index,value)

static void palLed_set_(cModule* mod, uint8_t state, int nodeId) {

	state = 0x7 & state;
	ledState[nodeId] = state;

	switch (state) {
	case 0:
		modifyDisplay(mod, "i", 1, "#808080");
		break;
	case 1:
		modifyDisplay(mod, "i", 1, "#0000FF");
		break;
	case 2:
		modifyDisplay(mod, "i", 1, "#00FF00");
		break;
	case 3:
		modifyDisplay(mod, "i", 1, "#00FFFF");
		break;
	case 4:
		modifyDisplay(mod, "i", 1, "#FF0000");
		break;
	case 5:
		modifyDisplay(mod, "i", 1, "#FF00FF");
		break;
	case 6:
		modifyDisplay(mod, "i", 1, "#FFFF00");
		break;
	case 7:
		modifyDisplay(mod, "i", 1, "#FFFFFF");
		break;
	default:
		ASSERT(false);
		break;
	}


}

static void palLed_toggle_(cModule* mod, uint8_t mask) {
	int nodeId = mod->getParentModule()->par("id");
	uint8_t state = ledState[nodeId];
	state = ~state;
	palLed_set_(mod, state, nodeId);

}

static void palLed_on_(cModule* mod, uint8_t mask) {
	int nodeId = mod->getParentModule()->par("id");
	uint8_t state = ledState[nodeId];
	state |= mask;
	palLed_set_(mod, state, nodeId);
}

static void palLed_off_(cModule* mod, uint8_t mask) {
	int nodeId = mod->getParentModule()->par("id");
	uint8_t state = ledState[nodeId];
	state &= ~mask;
	palLed_set_(mod, state, nodeId);
}

static void doOperation(LedCmd cmd, uint8_t mask) {
    cModule * m = getContextModule();
    if (m != NULL) {
        switch(cmd) {
            case LED_ON: palLed_on_(m, mask); break;
            case LED_OFF: palLed_off_(m, mask); break;
            case LED_TOGGLE: palLed_toggle_(m, mask); break;
            default: ASSERT(false); break;
        }
    } else {
        // can not do anything without a module belonging to some node
    }
}

void palLed_toggle(uint8_t mask) {
    doOperation(LED_TOGGLE, mask);
}

void palLed_on(uint8_t mask) {
    doOperation(LED_ON, mask);
}

void palLed_off(uint8_t mask) {
    doOperation(LED_OFF, mask);
}
