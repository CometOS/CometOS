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

#ifndef NETUTILS_H
#define NETUTILS_H

/**
 *  @file netutils.h
 *
 *  @author  Telematics Group
 *
 *  @date    2006-12-14
 */

/******** include files **********************************************/
#include <stdbool.h>

#ifdef _WIN32
#include <winsock.h>
#else
#ifdef __linux__
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#else
extern "C"
{
#include "fnet.h"
}
#define htons fnet_htons
#define htonl fnet_htonl
#define SHUT_RDWR SD_BOTH
#define EINPROGRESS FNET_ERR_INPROGRESS
#endif
#endif

/******** macro definitions ******************************************/
#ifdef __linux__
#define closesocket          close
#endif

#ifdef _WIN32
#define startSocketSession() _startWin32SocketSession()
#define stopSocketSession()  _stopWin32SocketSession()
#else
static inline bool startSocketSession() {return true;}
static inline void stopSocketSession()  {return;}
#endif

/******** public constants *******************************************/
#ifndef _WIN32
#define INVALID_SOCKET       (-1)
#endif

#ifdef __linux__
#define SOCKET_ERROR         (-1)
#endif

#ifndef IPPORT_USERRESERVED
#define IPPORT_USERRESERVED  (5000)  ///< up to this number, ports are reserved and should not be used
#endif
 
/******** public types ***********************************************/
/**
 * type for socket descriptor, depends on operating system
 */
#ifdef _WIN32
typedef SOCKET  socket_t;
typedef int     socklen_t;
#else
typedef int     socket_t;
#endif

/******** public global variables (extern) ***************************/
/* NONE */


/******** public function prototypes *********************************/
unsigned long lookupHostAddress(const char *hostName);

socket_t createClientSocket(const char *serverName, unsigned short portNumber, bool blocking = true);

socket_t createServerSocket(unsigned short portNumber);

socket_t acceptClientSocket(socket_t server);

bool shutdownAndCloseSocket(socket_t sock);

bool sendMessage(socket_t sock, const void *msg, int msgSize);

bool setNonBlockingSocket(socket_t sock);

int receiveMessage(socket_t sock, void *msg, int msgSize);


#ifdef _WIN32
bool _startWin32SocketSession(void);

void _stopWin32SocketSession(void);
#endif /* _WIN32 */

#endif /* NETUTILS_H */
/******** end of file netutils.h *************************************/
