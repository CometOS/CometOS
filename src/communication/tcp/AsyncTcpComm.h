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
#ifndef ASYNCTCPCOMM_H_
#define ASYNCTCPCOMM_H_

#include <list>
#include <string.h>
#include "LowerEndpoint.h"
#include "AsyncTcpAgent.h"



/**
 * This class provides capacities to connect nodes via TCP/IP.
 * AsyncTcpComm can be used as server and/or client.
 * Note that each instance runs new thread and works only
 * in conjunction with ConcRealTimeScheduler.
 */
class AsyncTcpComm: public cometos::LowerEndpoint, public AsyncTcpAgent {
public:

	AsyncTcpComm(node_t fallbackAddr = 0);

	void initialize();
	void finish();
#ifdef OMNETPP
	void connect(cometos::Message* msg);
#endif

	virtual void handleRequest(cometos::DataRequest* msg);

	virtual void receivedCallback(const socket_t& handler, uint8_t *buffer,
			uint8_t length);

private:
	node_t getAddr();
};

#endif /* ASYNCTCPCOMM_H_ */
