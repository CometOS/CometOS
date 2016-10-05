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

#ifndef ENDPOINTWRAP_H_
#define ENDPOINTWRAP_H_

#include "Endpoint.h"

/**
 * Endpoint Wrapper, which can be used as alternative base
 * for simple modules. Currently, mainly used for Python classes.
 */
class EndpointWrap : public cometos::Endpoint {
public:
	virtual ~EndpointWrap();

	EndpointWrap(const char* service_name = NULL);

	/**should be implemented by derived class*/
	virtual void initialize();

	/**should be implemented by derived class*/
	virtual void recvIndication(cometos::ByteVector& frame, int src, int dst);

	/**should be implemented by derived class*/
	virtual void recvConfirm(bool success);

	/**should be implemented by derived class*/
	virtual void timeout();


	void handleIndication(cometos::DataIndication* msg);

	void sendRequest(cometos::ByteVector& frame, int dst);

	void setTimer(uint16_t time);

	void timeout_(cometos::Message* msg);

	void handleResponse(cometos::DataResponse *resp);

};

#endif /* ENDPOINTWRAP_H_ */
