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

#include "TcpAgent.h"
#include <iostream>
#include <assert.h>
#include <string.h>
#include "cometos.h"
#include "OutputStream.h"

int TcpAgent::instCounter = 0;
using namespace std;

TcpAgent::TcpAgent() {

	if (instCounter == 0) {
		startSocketSession();
	}

	server = INVALID_SOCKET;

	instCounter++;
}

TcpAgent::~TcpAgent() {
	shutdown();

	instCounter--;
	if (instCounter == 0) {
		stopSocketSession();
	}

}

void TcpAgent::close(const socket_t& handler) {
	cometos::getCout() << "closing " << (int32_t)handler << cometos::endl;
	conn.erase(handler);
	shutdownAndCloseSocket(handler);
}

void TcpAgent::pollAll() {
	uint8_t buffer[TCP_AGENT_BUFFER_SIZE];
	std::set<socket_t> toRemove;
	if (server != INVALID_SOCKET) {
		socket_t client = acceptClientSocket(server);
		if (INVALID_SOCKET != client) {
			setNonBlockingSocket(client);
			cometos::getCout() << "Accepted client connection" << cometos::endl;
			conn.insert(client);
		}
	}

	for (std::set<socket_t>::iterator it = conn.begin(); it != conn.end();
			it++) {

		int c = receiveMessage((*it), buffer, 1);

		if (c > 0) {

			int max = (uint8_t) *buffer;
			uint8_t offset = 0;
			assert(max < TCP_AGENT_BUFFER_SIZE);
			int timeout = 0;
			while (1) {
				c = 0;
				c = receiveMessage((*it), buffer + offset, max - offset);
				if (c > 0) {
					assert(max>=c);
					offset += c;
					if (max == offset) {
						receivedCallback((*it), buffer, (uint8_t) max);
						break;
					}
				} else if (c == 0) {
					cometos::getCout() << "c == 0" << cometos::endl;
					toRemove.insert(*it);
					break;
				} else {
					if (timeout > 100) {
						cometos::getCout() << "timeout > 100ms" << cometos::endl;
						toRemove.insert(*it);
						break;
					}
					timeout++;
#ifdef _WIN32
					Sleep(1);
#else
#ifdef __linux__
					usleep(1000);
#else
					fnet_timer_delay(fnet_timer_ms2ticks(1));
#endif
#endif
				}
			}
		} else if (c < 0) {

		} else {
#ifdef _WIN32
            cometos::getCout() << "c == 0 outer loop" << cometos::endl;
            toRemove.insert(*it);
#else
#ifdef __linux__
            cometos::getCout() << "c == 0 outer loop" << cometos::endl;
            toRemove.insert(*it);
#else
            // none
#endif
#endif
		}

	}

	while (!toRemove.empty()) {
		close(*toRemove.begin());
		toRemove.erase(toRemove.begin());
	}

}

bool TcpAgent::listen(int port) {
	// check whether server is already started
	if (INVALID_SOCKET != server) {
		return false;
	}

	server = createServerSocket(port);
	setNonBlockingSocket(server);
	if (INVALID_SOCKET != server) {
		return true;
	}

	return false;
}

void TcpAgent::shutdown() {
	cometos::getCout() << "complete shutdown" << cometos::endl;
	for (set<socket_t>::iterator it = conn.begin(); it != conn.end(); it++) {
		shutdownAndCloseSocket(*it);
	}
	conn.clear();

	if (server != INVALID_SOCKET) {
		cometos::getCout() << "Shutdown server" << cometos::endl;
		shutdownAndCloseSocket(server);
		server = INVALID_SOCKET;
	}

}

bool TcpAgent::connect(const char* addr, int port) {
	socket_t client = createClientSocket(addr, port);
	if (INVALID_SOCKET != client) {
		setNonBlockingSocket(client);
		conn.insert(client);
		//startThread();
		return true;
	}
	return false;
}

bool TcpAgent::send(const socket_t& handler, uint8_t *buffer, uint8_t length) {
	uint8_t buff[length+1];

	buff[0]=length;
	memcpy(buff+1,buffer,length);

	if (sendMessage(handler, buff, length+1) == false) {
		conn.erase(handler);
		cometos::getCout() << "sendMessage failed" << cometos::endl;
		shutdownAndCloseSocket(handler);
		return false;
	}
	return true;
}
