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

#include "ThreadTest.h"
#include <iostream>
#include "palExec.h"

#ifdef _WIN32
#include "windows.h"
#endif

using namespace std;

Define_Module(cometos::ThreadTest);

static void threadTestFunction(thread_handler_t h, void* p) {
	cometos::ThreadTest *inst = (cometos::ThreadTest *) (p);

	while (!thread_receivedStopSignal(h)) {
#ifdef _WIN32
		Sleep(400);
#else
		usleep(400000);
#endif
		cout << "try async schedule" << endl;

		palExec_atomicBegin();
		inst->scheduleTask();
		palExec_atomicEnd();
		palExec_wakeup();
	}

}

namespace cometos {

void ThreadTest::initialize() {
	thread = thread_run(threadTestFunction, this);

}

void ThreadTest::finalize() {
	thread_stopAndClose(thread);
}



void ThreadTest::scheduleTask() {
	// NEVER USE CRITICAL SECTION IN THIS FUNCTION
	// since Enter_Method_Silent() creates a local instance
	// which has a destructor that changes core system
	Enter_Method_Silent();
	schedule(new Message, &ThreadTest::timeout);
}

void ThreadTest::timeout(Message* msg) {
	cout << "timeout" << endl;
	delete msg;
}

} /* namespace cometos */
