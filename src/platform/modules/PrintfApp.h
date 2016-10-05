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
#ifndef PRINTFAPP_H_
#define PRINTFAPP_H_

#include "Endpoint.h"
#include "AirString.h"
#include "Arbiter.h"

#ifndef BASESTATION_ADDR
#define BASESTATION_ADDR 0
#endif

/**
 * You should only initialize one instance. We can not use a singleton class, because
 * of the poor handling of static initialization by the C++ Compiler.
 *
 * */
class PrintfApp  : public cometos::Endpoint {
public:

	PrintfApp(const char * name = NULL, node_t printfDest = BASESTATION_ADDR);

	virtual void handleIndication(cometos::DataIndication* msg);

	void initialize();


    void flush(cometos::Event* event = nullptr);

    static PrintfApp* getInstance();

    void handleResponse(cometos::DataResponse* resp);

private:
	void forwardToSink(cometos::AirString *data);
	uint8_t sequence;
	node_t dst;
	cometos::EventQueue flushEvents;
    bool transmitting;
};

#endif /* PRINTFAPP_H_ */
