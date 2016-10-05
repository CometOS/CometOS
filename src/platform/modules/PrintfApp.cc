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
 * @author Stefan Unterschuetz
 */

/*INCLUDES-------------------------------------------------------------------*/

#include <string.h>
#include "PrintfApp.h"
#include "logging.h"
#include "AirString.h"
#include "OutputStream.h"

#include "palId.h"
#include "palExec.h"

/*VARIABLES------------------------------------------------------------------*/

static cometos::AirString str;
static uint8_t length;

static PrintfApp* &getPrintfApp() {
	static PrintfApp* printfApp = nullptr;
	return printfApp;
}

PrintfApp* PrintfApp::getInstance() {
    return getPrintfApp();
}

/*printf output function*/
static void PrintfApp_putchar(char c) {
	PrintfApp* app = getPrintfApp();
	if (app == nullptr) {
		return;
	}

	str.getStr_unsafe()[length] = (uint8_t) c;
	length++;
	// send data if maximum length is received or line break;
	if (c == '\n' || length == (cometos::AirString::MAX_LEN - 1)) {
        app->flush();
	}
}

static void PrintfApp_flush(cometos::Event* event) {
    PrintfApp* app = getPrintfApp();
    app->flush(event);
}

namespace cometos {

OutputStream &getCout() {
    static OutputStream cout(PrintfApp_putchar,PrintfApp_flush);
	return cout;
}
}

void PrintfApp::initialize() {
	Endpoint::initialize();
	ASSERT(getPrintfApp()==nullptr);
    transmitting = false;
	getPrintfApp() = this;
}

void PrintfApp::forwardToSink(cometos::AirString *data) {
    cometos::Airframe *frame = new cometos::Airframe;
	(*frame) << (*data) << sequence;
	sequence++;
	Endpoint::sendRequest(new cometos::DataRequest(dst, frame, createCallback(&PrintfApp::handleResponse)));
}

void PrintfApp::handleResponse(cometos::DataResponse* resp) {
    palExec_atomicBegin();
    transmitting = false;
    flushEvents.scheduleAllEvents();
    palExec_atomicEnd();
    delete(resp);
}

void PrintfApp::handleIndication(cometos::DataIndication* msg) {
    //nodes are not meant to actually receive printfapp messages
	delete msg;
}

PrintfApp::PrintfApp(const char * name, node_t printfDest) :
        cometos::Endpoint(name), sequence(0), dst(printfDest) {
	length = 0;
}

void PrintfApp::flush(cometos::Event* event) {
    palExec_atomicBegin();
    if(event != nullptr) {
        flushEvents.push(event);
    }

    if(length > 0) {
        transmitting = true;
        str.getStr_unsafe()[length] = 0;
        forwardToSink(&str);
        length = 0;
        palExec_atomicEnd();
        return;
    }
    else if(!transmitting) {
        flushEvents.scheduleAllEvents();
    }
    palExec_atomicEnd();
}

