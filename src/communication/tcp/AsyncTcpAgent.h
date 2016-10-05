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

#ifndef ASYNCTCPAGENT_H_
#define ASYNCTCPAGENT_H_

#include <stdint.h>
#include <map>
#include <list>
#include "netutils.h"
#include "threadutils.h"

#define  ASYNC_TCP_AGENT_BUFFER_SIZE	255

/**
 * This class manages multiple TCP IP Connection and provides facility to
 * run server.
 *
 * An instance of this class uses one thread to perform all operations.
 */
class AsyncTcpAgent {
public:
	AsyncTcpAgent();
	virtual ~AsyncTcpAgent();

	/**
	 * Callback when data is received.
	 *
	 * @handler id of connection
	 */
	virtual void receivedCallback(const socket_t& handler, uint8_t *buffer,
			uint8_t length)=0;

	/**Sends data to the node specified with handler.
	 * If handler does not exist or connection closed, then
	 * this function returns false.
	 */
	bool send(const socket_t& handler, uint8_t *buffer, uint8_t length);

	void sendToAll(uint8_t *buffer, uint8_t length);

	/**Class accepted TCP IP connection and assigned a handelr to it
	 */
	//virtual bool acceptedCallback(socket_t& handler)=0;


	//virtual void closedCallback(socket_t& handler)=0;

	/**Listens for connection
	 */
	bool listen(int port);

	void shutdown();

	void startClientThread(socket_t &client);

	socket_t findClient(thread_handler_t);

	bool connect(const char* addr, int port);

	/**Contains all open connections*/
	std::map<socket_t,thread_handler_t> conn;

	thread_handler_t serverT;
	socket_t server;

	thread_mutex_t mutex;

	static void shutdownAll();

private:
	static std::list<AsyncTcpAgent*> &  getInstList();
	static int instCounter;
};

#endif /* ASYNCTCPAGENT_H_ */
