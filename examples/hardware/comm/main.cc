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

/**@file This example shows how to build up larger protocol stacks.
 *
 * @author Stefan Unterschuetz
 */
#include "cometos.h"

#include "LayerExample.h"
#include "EndpointExample.h"
#include "LowerEndpointExample.h"
#include "SelectorExample.h"
#include "Dispatcher.h"


/*PROTOTYPES-----------------------------------------------------------------*/

EndpointExample upper("up");
SelectorExample sel;
LayerExample layer0("la0");
LayerExample layer1("la1");
LayerExample layer2("la2");
cometos::Dispatcher<3> dispatcher;
LowerEndpointExample lower("low");


int main() {

	// connect gates
	upper.gateReqOut.connectTo(sel.gateReqIn);
	sel.gateIndOut.connectTo(upper.gateIndIn);

	sel.gateReqOut.get(0).connectTo(layer0.gateReqIn);
	layer0.gateIndOut.connectTo(sel.gateIndIn.get(0));

	sel.gateReqOut.get(1).connectTo(layer1.gateReqIn);
	layer1.gateIndOut.connectTo(sel.gateIndIn.get(1));

	sel.gateReqOut.get(2).connectTo(layer2.gateReqIn);
	layer2.gateIndOut.connectTo(sel.gateIndIn.get(2));

	layer0.gateReqOut.connectTo(dispatcher.gateReqIn.get(0));
	dispatcher.gateIndOut.get(0).connectTo(layer0.gateIndIn);

	layer1.gateReqOut.connectTo(dispatcher.gateReqIn.get(1));
	dispatcher.gateIndOut.get(1).connectTo(layer1.gateIndIn);

	layer2.gateReqOut.connectTo(dispatcher.gateReqIn.get(2));
	dispatcher.gateIndOut.get(2).connectTo(layer2.gateIndIn);

	dispatcher.gateReqOut.connectTo(lower.gateReqIn);
	lower.gateIndOut.connectTo(dispatcher.gateIndIn);

	cometos::initialize();
	cometos::run();

	return 0;
}
