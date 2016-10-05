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
 *  @file netutils.c
 *
 *  This module provides functionality to communicate between a client
 *  and a server.
 *
 *  @author  Telematics Group
 *
 *  @date    2007-10-11
 */

/********* include files *********************************************/
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>

#include "cometos.h"

#define dtrace(x)
// cometos::getCout()<<x<<cometos::endl;
#define derror(x) std::cerr<<x<<std::endl;

#ifdef _WIN32
#include <winsock2.h>
#else
#ifdef __linux__
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <fcntl.h>
#endif
#endif

#include "netutils.h"
#include "logging.h"

/********* private constants *****************************************/
#ifdef _WIN32
#define WS_MAJOR_VERSION   2        ///< major version of Winsock API
#define WS_MINOR_VERSION   2        ///< minor version of Winsock API
#define SHUT_RDWR          SD_BOTH  ///< @c SHUT_RDWR is POSIX standard
#endif

#define QUEUE_LENGTH       5        ///< max queue length of pending connections
/********* private types *********************************************/
/* NONE */

/********* public global variables (as extern in header file) ********/
/* NONE */

/********* private global variables **********************************/
/* NONE */

/********* private function prototypes *******************************/
/* NONE */

/********* function definitions **************************************/

/**
 *  This function returns an integer (unsigned long) representation
 *  of an IP address, given as a FQN or a dotted IP address.
 *
 *  @param  hostName     name of a host as FQN or dotted IP address
 *
 *  @return  The return value is an integer value of a dotted IP address if
 *           the hostname exists (valid DNS entry) or the hostname is given
 *           as a dotted IP address. Otherwise the function returns
 *           @c INADDR_NONE.
 */
#if defined(_WIN32) || defined(__linux__)
unsigned long lookupHostAddress(const char *hostName) {
	unsigned long addr; /* inet address of hostname */
	struct hostent *host; /* host structure for DNS request */

	dtrace("entering lookupHostAddress()");

	if (NULL == hostName) {
		derror("invalid parameter");
		dtrace("leaving lookupHostAddress()");
		return (INADDR_NONE);
	}

	dtrace("looking for host " << hostName);

	addr = inet_addr(hostName);

	if (INADDR_NONE == addr) {
		/* hostName isn't a dotted IP, so resolve it through DNS */
		host = gethostbyname(hostName);
		if (NULL != host) {
			addr = *((unsigned long *) host->h_addr_list[0]);
		}
	}

	dtrace("leaving lookupHostAddress()");
	return (addr);
}
#endif
/******** end of function lookupHostAddress **************************/

/**
 *  This function creates a new client internet domain socket (TCP/IP)
 *  and connects to a service on an internet host.
 *
 *  @param  serverName   hostname of a server
 *  @param  portNumber   port number of service
 *
 *  @return  The function returns a socket handle if the socket can be created
 *           and connected to a service. If an error occurred, the function
 *           returns @c INVALID_SOCKET.
 */
socket_t createClientSocket(const char *serverName, unsigned short portNumber, bool blocking) {
	struct sockaddr_in srvAddr; /* server's internet socket address */
	socket_t sock; /* file descriptor for client socket */

	dtrace("entering createClientSocket()");

	/* get the IP address of the server host */
#if defined(_WIN32) || defined(__linux__)
	unsigned long ipAddress; /* internet address */
	if (INADDR_NONE == (ipAddress = lookupHostAddress(serverName))) {
		derror("lookupHostAddress() failed");
		dtrace("leaving createClientSocket() with INVALID_SOCKET");
		return (INVALID_SOCKET);
	}

	/* fill the server address structure */
	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(portNumber);
	srvAddr.sin_addr.s_addr = ipAddress;
#else
    	if(fnet_inet_ptos(serverName, (struct sockaddr *)&srvAddr) == FNET_ERR)
    	{
		dtrace("Calculating address failed");
		return (INVALID_SOCKET);
    	}

	srvAddr.sin_port = htons(portNumber);
#endif

	dtrace("create the client socket");

	/* create the client socket */
	if (INVALID_SOCKET == (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP))) {
		derror("socket creation failed");
		dtrace("leaving createClientSocket() with INVALID_SOCKET");
		return (INVALID_SOCKET);
	}

        if(!blocking) {
                setNonBlockingSocket(sock);
        }

	dtrace("trying to connect "<<serverName<<" on port "<< portNumber);

	/* try to connect to the server socket */
	if(SOCKET_ERROR == connect(sock, (struct sockaddr *) &srvAddr, sizeof(srvAddr))) {
#ifndef FNET
            if(errno == EINPROGRESS) {
                dtrace("EINPROGRESS in connect() - selecting"); 
                do { 
                    struct timeval tv; 
                    tv.tv_sec = 5; 
                    tv.tv_usec = 0; 
                    fd_set myset;  
                    FD_ZERO(&myset); 
                    FD_SET(sock, &myset); 
                    int res = select(sock+1, NULL, &myset, NULL, &tv); 
                    if (res < 0 && errno != EINTR) { 
                        dtrace("Error connecting " << errno << " " << strerror(errno));
                        goto error;
                    } 
                    else if (res > 0) { 
                        // Socket selected for write 
                        socklen_t lon = sizeof(int); 
                        int valopt;
                        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &lon) < 0) {
                            dtrace("Error in getsockopt() " << errno << " - " << strerror(errno));
                            goto error;
                        } 
                        // Check the value returned... 
                        if (valopt) { 
                            dtrace("Error in delayed connection() " << valopt << " - " << strerror(valopt));
                            goto error;
                        } 
                        break; 
                    } 
                    else { 
                        dtrace("Timeout in select() - Cancelling!");
                        goto error;
                    } 
                } while (1); 
            }
            else
#endif
                goto error;
	}

#ifdef FNET
    {
        int i = 0;
        const int timeout = 2000000;
        /* wait until connection is established */
        for(i = 0; i < timeout; i++) {
            fnet_socket_state_t state;
            int len = sizeof(fnet_socket_state_t);
            getsockopt(sock, SOL_SOCKET, SO_STATE, (char*)&state, &len);

            dtrace("state " << (int32_t)state);

            if(state == SS_UNCONNECTED) {
                derror("connect() failed");
                closesocket(sock);
                dtrace("leaving createClientSocket() with INVALID_SOCKET");
                return (INVALID_SOCKET);
            }

            if(state != SS_CONNECTING) {
                break;
            }
        }

        if(i >= timeout) {
            derror("connect() failed");
            closesocket(sock);
            dtrace("leaving createClientSocket() with INVALID_SOCKET");
            return (INVALID_SOCKET);
        }
    }
#endif

	dtrace("leaving createClientSocket() with socket id "<<(uint16_t)(sock));
	return (sock);

error:
        derror("connect() failed");
        closesocket(sock);
        dtrace("leaving createClientSocket() with INVALID_SOCKET");
        return (INVALID_SOCKET);
}


void signalHandler( int signum ) {

}

/******** end of function createClientSocket *************************/

/**
 *  This function creates a new server internet domain socket (TCP/IP).
 *
 *  @param  portNumber   port number on which to listen for incoming packets
 *
 *  @return  The function returns a socket handle if the socket can be created.
 *           If an error occurs, the function returns @c INVALID_SOCKET.
 */
socket_t createServerSocket(unsigned short portNumber) {
	struct sockaddr_in srvAddr; /* server's internet socket address */
	socket_t sock; /* file descriptor for server socket */

	dtrace("entering createServerSocket()");

	/* create the server socket */
	if (INVALID_SOCKET == (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP))) {
		derror("socket creation failed");
		dtrace("leaving createServerSocket() with INVALID_SOCKET");
		return (INVALID_SOCKET);
	}

#if defined(_WIN32) || defined(__linux__)
#ifdef _WIN32
	char yes = 1;
#else
	int yes = 1;
#endif
	if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
		derror("set socket options failed");
		return (INVALID_SOCKET);
	}
#endif

	/* fill the server address structure */
	/* first of all, zero srvAddr, so that we have a defined status */
	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(portNumber);
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* try to bind socket to the specified server port */
	if (SOCKET_ERROR
			== bind(sock, (struct sockaddr *) &srvAddr, sizeof(srvAddr))) {
		derror("bind() failed!");
		closesocket(sock);
		dtrace("leaving createServerSocket() with INVALID_SOCKET");
		return (INVALID_SOCKET);
	}

	if (SOCKET_ERROR == listen(sock, QUEUE_LENGTH)) {
		derror("listen() failed!");
		shutdownAndCloseSocket(sock);
		dtrace("leaving createServerSocket() with INVALID_SOCKET");
		return (INVALID_SOCKET);
	}

	dtrace("server started at port "<<portNumber);

	dtrace("leaving createServerSocket()");
	return (sock);
}
/******** end of function createServerSocket *************************/

/**
 *  This function sends a message of a given size to a communication
 *  partner via a connected socket.
 *
 *  @param  sock         socket for sending the message
 *  @param  msg          pointer to the message buffer which should be sent
 *  @param  msgSize      size of the message buffer
 *
 *  @return  The function returns @c true if the message was sent to the server,
 *           otherwise the function returns @c false.
 */bool sendMessage(socket_t sock, const void *msg, int msgSize) {
	dtrace("entering sendMessage()");

	/* check if parameters are valid */
	if (NULL == msg) {
		derror("invalid message buffer!");
		dtrace("leaving sendMessage()");
		return (false);
	}

	if (0 >= msgSize) {
		derror("invalid message size "<<msgSize);
		dtrace("leaving sendMessage()");
		return (false);
	}

	dtrace("sending message of size "<< (uint32_t)msgSize);

	/* now send the message */
	if (msgSize != send(sock, (const char *) msg, msgSize, 0)) {
		derror("sending message failed");
		dtrace("leaving sendMessage()");
		return (false);
	}

	dtrace("leaving sendMessage()");
	return (true);
}
/******** end of function sendMessage ********************************/

/**
 *  This functions receives a message from a communication partner and
 *  stores it into a given message buffer.
 *  The function blocks until all bytes have been successfully received,
 *  or an error occurred.
 *
 *  @param       sock       socket for receiving the message
 *  @param[out]  msg        message buffer for receiving the message
 *  @param       msgSize    size of the buffer to contain the message
 *
 *  @return  The function returns @c true if all things went well. Otherwise
 *           the function returns @c false.
 */
int receiveMessage(socket_t sock, void *msg, int msgSize) {

	dtrace("entering receiveMessage()");

	/* check if parameters are valid */
	if (NULL == msg) {
		derror("invalid message buffer!");
		dtrace("leaving receiveMessage()");
		return (false);
	}

	if (0 >= msgSize) {
		derror("invalid message size!");
		dtrace("leaving receiveMessage()");
		return (false);
	}

	dtrace("trying to receive a message of size "<< (uint32_t)msgSize);

	return recv(sock, (char *) msg, msgSize, 0);

}

socket_t acceptClientSocket(socket_t server) {
	dtrace("trying to accept new connection ");
	//	struct sockaddr_in cli;
	//	socklen_t cli_size = sizeof(cli);

	return accept(server, (sockaddr*) NULL, NULL);
}

/******** end of function receiveMessage *****************************/

/**
 *  This function shuts down and closes a given socket.
 *
 *  @param  sock         socket to be closed
 *
 *  @return  if all things went ok, this function returns @c true, otherwise
 *           @c false
 */
bool shutdownAndCloseSocket(socket_t sock) {
	bool status = true;

	dtrace("entering shutdownAndCloseSocket()");

	if (SOCKET_ERROR == shutdown(sock, SHUT_RDWR)) {
		derror( "shutdown() failed");
		status = false;
	}

	if (SOCKET_ERROR == closesocket(sock)) {
		derror( "closesocket() failed");
		status = false;
	}

	dtrace("leaving shutdownAndCloseSocket()");
	return (status);
}

bool setNonBlockingSocket(socket_t sock) {
#ifdef _WIN32
	u_long iMode=1;
	ioctlsocket(sock,FIONBIO,&iMode);
	return true;
#else
#ifdef __linux__
	int opts;

	opts = fcntl(sock, F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(EXIT_FAILURE);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
	}
	return true;
#else
	return true; // in FNET there are only non-blocking sockets
#endif
#endif
}

/******** end of function shutdownAndCloseSocket *********************/

#ifdef _WIN32

/**
 *  This function initializes the Win32 Socket API.
 *
 *  @return  if all things went ok, this function returns @c true, otherwise
 *           @c false
 */bool _startWin32SocketSession(void) {
	WORD requestedVersion;
	WSADATA wsaData;

	dtrace("entering _startWin32SocketSession()");

	requestedVersion = MAKEWORD(WS_MAJOR_VERSION, WS_MINOR_VERSION);

	if (0 != WSAStartup(requestedVersion, &wsaData)) {
		derror(
				"WSAStartup() failed");dtrace("leaving _startWin32SocketSession() with error");
		return (false);
	}

	/* Confirm that the Windows Socket DLL supports 1.1. */
	/* Note that if the DLL supports versions greater    */
	/* than 1.1 in addition to 1.1, it will still return */
	/* 1.1 in wVersion since that is the version we      */
	/* requested.                                        */

	if (WS_MINOR_VERSION != LOBYTE(wsaData.wVersion) || WS_MAJOR_VERSION
			!= HIBYTE(wsaData.wVersion)) {
		derror(
				"Windows Socket DLL does not support the requested version");
		_stopWin32SocketSession();
		dtrace("leaving _startWin32SocketSession() with error");
		return (false);
	}

	WSASetLastError(0); /* reset the error code */

	dtrace("leaving _startWin32SocketSession()");
	return (true);
}
/******** end of function _startWin32SocketSession *******************/

/**
 *  This function terminates the Win32 Socket API.
 *  No future API calls are allowed.
 */
void _stopWin32SocketSession(void) {
	dtrace("entering _stopWin32SocketSession()");

	if (SOCKET_ERROR == WSACleanup()) {
		derror(
				"WSACleanup() failed");
	}

	dtrace("leaving _stopWin32SocketSession()");
	return;
}
/******** end of function _stopWin32SocketSession ********************/

#endif /* _WIN32 */

/******** end of file netutils.c *************************************/
