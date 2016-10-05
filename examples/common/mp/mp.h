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

/**This file declares classes used for the message passing example.
 * @author Stefan Unterschütz
 */

#ifndef MP_H_
#define MP_H_

/**@file
 * Message passing demo based on CometOS.
 */

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "palLed.h"


/*CLASSES--------------------------------------------------------------------*/

/**New message class.
 */
class MyMessage: public cometos::Message {
};

/**A sender.
 */
class MySender: public cometos::Module {
public:
    cometos::OutputGate<MyMessage> gateOut;

	MySender() :
		gateOut(this, "gateOut") {
	}

	void initialize() {
		schedule(new cometos::Message, &MySender::traffic);
	}

	void traffic(cometos::Message* timer) {
		palLed_toggle(2);
		gateOut.send(new MyMessage);
		schedule(timer, &MySender::traffic, 200);
	}
};

/**A receiver.
 * */
class MyReceiver: public cometos::Module {
public:

    cometos::InputGate<MyMessage> gateIn;

	MyReceiver() :
		gateIn(this, &MyReceiver::handle, "gateIn") {
	}

	void handle(MyMessage *msg) {
		palLed_toggle(1);
		delete msg;
	}
};


#endif /* MP_H_ */
