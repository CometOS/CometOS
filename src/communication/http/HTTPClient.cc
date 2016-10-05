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
 * @author Florian Meier
 */

#include "HTTPClient.h"
#include "cometos.h"
#include "OutputStream.h"

#include <sstream>

using namespace std;
using namespace cometos;

bool HTTPClient::getRequest(const char* addr, unsigned short port, string url, stringstream& resp, bool headOnly, string additionalHeader) {
	socket_t socket = createClientSocket(addr, port, false);

	if(socket == INVALID_SOCKET) {
		cometos::getCout() << "No connection to HTTP server!" << cometos::endl;	
		return false;
	}

	stringstream req;

        if(headOnly) {
        	req << "HEAD ";
        }
        else {
        	req << "GET ";
        }
        req << url;
	req << " HTTP/1.0\n";
        req << additionalHeader;
        req << "\n";
	
	sendMessage(socket, req.str().c_str(), req.str().length());

	char buffer[100];
	int c;
	while(1) {
		c = receiveMessage(socket, buffer, sizeof(buffer));
		//cometos::getCout() << "c: " << (int32_t)c << cometos::endl;

#ifdef FNET
		if(c <= -1) {
#else
		if(c <= 0) {
#endif
			break;
		}
		resp.write(buffer,c);
	}

	shutdownAndCloseSocket(socket);

	return true;
}

bool HTTPClient::postRequest(const char* addr, unsigned short port, std::string url, std::stringstream& req, std::stringstream& resp) {
	socket_t socket = createClientSocket(addr, port, false);

	if(socket == INVALID_SOCKET) {
		cometos::getCout() << "No connection to HTTP server!" << cometos::endl;	
		return false;
	}

	stringstream reqnew;
	reqnew << "POST " << url;
	reqnew << " HTTP/1.0\n";
	reqnew << "Content-Type: application/x-www-form-urlencoded\n";
	reqnew << "Content-Length: " << req.str().length() << "\n\n";
	reqnew << req.str() << "\n\n";
	
	sendMessage(socket, reqnew.str().c_str(), reqnew.str().length());

	char buffer[100];
	int c;
	while(1) {
		c = receiveMessage(socket, buffer, sizeof(buffer));
		//cometos::getCout() << "c: " << (int32_t)c << cometos::endl;

#ifdef FNET
		if(c <= -1) {
#else
		if(c <= 0) {
#endif
			break;
		}
		resp.write(buffer,c);
	}

	shutdownAndCloseSocket(socket);

	return true;
}

