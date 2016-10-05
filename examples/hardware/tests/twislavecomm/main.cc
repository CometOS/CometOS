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


/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "TwiComm.h"
#include "Endpoint.h"
#include "DummyModule.h"

using namespace cometos;
using namespace std;

/*PROTOTYPES-----------------------------------------------------------------*/

// sign of life output
TaskScheduler scheduler;
void aliveSignal();
SimpleTask task(aliveSignal);
void aliveSignal() {
	cometos::getCout() << "Huhu" << cometos::endl;
	cometos::getScheduler().add(task, 3000);
}


// module instantiation


int main() {
	cometos::getCout() << "Booting" << cometos::endl;

	TwiComm comm(0, false, 0x12);
	DummyModule bounce("sb");

	bounce.gateReqOut.connectTo(comm.gateReqIn);
	comm.gateIndOut.connectTo(bounce.gateIndIn);

	cometos::initialize();
	cometos::getScheduler().add(task, 0);

	cometos::run();
	return 0;
}

