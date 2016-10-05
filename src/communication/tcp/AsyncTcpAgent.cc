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

#include "AsyncTcpAgent.h"
#include <iostream>
#include <csignal>
#include <assert.h>
#include <string.h>
#include "palExec.h"
#include <stdlib.h>

int AsyncTcpAgent::instCounter = 0;


using namespace std;

static void signalHandler(int signum) {
	// TODO sometimes caused doulbe free() system error
	// AsyncTcpAgent::shutdownAll();
	exit(signum);
}

static void serverThread(thread_handler_t h, void* p) {
	AsyncTcpAgent *inst = (AsyncTcpAgent *) (p);
	cout << "Start Server Thread" << endl;

	while (!thread_receivedStopSignal(h)) {
		socket_t client = acceptClientSocket(inst->server);
		if (INVALID_SOCKET == client) {
			cout << "Server socket closed, stop server" << endl;
			return;
		}

		cout << "Accepted client connection " << (int) client << endl;
		inst->startClientThread(client);
	}

	cout << "Stop Server Thread" << endl;

}

static void clientThread(thread_handler_t h, void* p) {
	uint8_t buffer[ASYNC_TCP_AGENT_BUFFER_SIZE];
	AsyncTcpAgent *inst = (AsyncTcpAgent *) (p);
	socket_t client = inst->findClient(h);
	cout << "Start Client Thread" << endl;

	while (!thread_receivedStopSignal(h)) {

		int c = receiveMessage(client, buffer, 1);
		int max = *buffer;

		if (c > 0) {

			assert(max < ASYNC_TCP_AGENT_BUFFER_SIZE);

			c = receiveMessage(client, buffer, max);
			if (max != c) {
				break;
			}

#ifdef OMNETPP
			palExec_atomicBegin();
#endif
			inst->receivedCallback(client, buffer, (uint8_t) max);
#ifdef OMNETPP
			palExec_atomicEnd();
#endif
			palExec_wakeup();

		} else {
			break;
		}
	}
	cout << "Stop Client Thread" << endl;

	thread_lockMutex(inst->mutex);

	// no stop signal was send by other thread, thus make own cleanup
	if (!thread_receivedStopSignal(h)) {
		thread_close(h);
		shutdownAndCloseSocket(client);
	}
	inst->conn.erase(client);
	thread_unlockMutex(inst->mutex);

	return;
}

void AsyncTcpAgent::shutdownAll() {
	std::list<AsyncTcpAgent*> & list = getInstList();
	std::list<AsyncTcpAgent*>::iterator i;
	for (i = list.begin(); i != list.end(); i++) {
		(*i)->shutdown();
	}
}

void AsyncTcpAgent::sendToAll(uint8_t *buffer, uint8_t length) {
	// broadcast data to all connected clients
	for (map<socket_t, thread_handler_t>::iterator it = conn.begin(); it
			!= conn.end(); it++) {
		send(it->first, buffer, length);
	}
}

void AsyncTcpAgent::startClientThread(socket_t &client) {
	if (conn.count(client) != 0) {
		return;
	}
	thread_lockMutex(mutex);
	conn[client] = thread_run(clientThread, this);
	thread_unlockMutex(mutex);
}

std::list<AsyncTcpAgent*> & AsyncTcpAgent::getInstList() {
	static std::list<AsyncTcpAgent*> instList;
	return instList;
}

AsyncTcpAgent::AsyncTcpAgent() :
	serverT(NULL) {
	mutex = thread_createMutex();

	if (instCounter == 0) {
		signal(SIGINT, signalHandler);
		startSocketSession();
	}

	server = INVALID_SOCKET;

	instCounter++;
	getInstList().push_back(this);
}

AsyncTcpAgent::~AsyncTcpAgent() {
	shutdown();

	thread_destroyMutex(mutex);

	getInstList().remove(this);
	instCounter--;
	if (instCounter == 0) {
		stopSocketSession();
	}
}

socket_t AsyncTcpAgent::findClient(thread_handler_t h) {
	for (std::map<socket_t, thread_handler_t>::iterator it = conn.begin(); it
			!= conn.end(); it++) {
		if (it->second == h) {
			return it->first;
		}
	}
	return 0;
}

bool AsyncTcpAgent::listen(int port) {
	// check whether server is already started
	cout << "try to start server on port " << port << endl;

	server = createServerSocket(port);
	if (INVALID_SOCKET == server) {
		cout << "starting server failed" << endl;
		return false;
	}

	serverT = thread_run(serverThread, this);

	return true;
}

void AsyncTcpAgent::shutdown() {
	cout << "Shutdown AsyncTcpAgent" << endl;

	if (server != INVALID_SOCKET) {
		cout << "shutdown server" << endl;
		shutdownAndCloseSocket(server);
		server = INVALID_SOCKET;
	}

	if (serverT) {
		thread_stopAndClose(serverT);
	}

	thread_lockMutex(mutex);
	while (conn.size() > 0) {
		socket_t client = conn.begin()->first;
		thread_handler_t handler= conn.begin()->second;
		thread_unlockMutex(mutex);
		shutdownAndCloseSocket(client);
		thread_stopAndClose(handler);
		thread_lockMutex(mutex);
	}
	thread_unlockMutex(mutex);

	cout << "Shutdown done" << endl;
}

bool AsyncTcpAgent::connect(const char* addr, int port) {
	socket_t client = createClientSocket(addr, port);
	if (INVALID_SOCKET != client) {
		startClientThread(client);
		return true;
	}
	return false;
}

bool AsyncTcpAgent::send(const socket_t& handler, uint8_t *buffer,
		uint8_t length) {

	uint8_t buff[length + 1];
	buff[0] = length;
	memcpy(buff + 1, buffer, length);

	if (sendMessage(handler, buff, length + 1) == false) {
		conn.erase(handler);
		shutdownAndCloseSocket(handler);
		return false;
	}
	return true;
}

