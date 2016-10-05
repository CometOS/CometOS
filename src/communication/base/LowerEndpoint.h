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

#ifndef LOWERENDPOINT_H_
#define LOWERENDPOINT_H_


/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "RemoteModule.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"


/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

/**
 * Base class for lower communication endpoints (e.g., Mac Layer). DataRequest
 * and DataIndication are used as exchange format.
 */
class LowerEndpoint : public RemoteModule {
public:

	LowerEndpoint(const char *name=NULL);

	/**Handler for DataRequest messages. This method should
	 * be overridden.
	 *
	 * @param msg 	valid pointer to DataRequest message (ownership is received)
	 * @param gate	pointer to input gate (= gateReqIn)
	 */
	virtual void handleRequest(DataRequest* msg);

	/**
	 * Sends DataIndication message to connected module.
	 *
	 * @param request	valid pointer to DataIndication object (ownership is passed)
	 * @param ms		sending offset in milliseconds
	 */
	void sendIndication(DataIndication* indication, timeOffset_t offset=0);

	InputGate<DataRequest> gateReqIn;

	OutputGate<DataIndication>  gateIndOut;
};

} /* namespace cometos */
#endif /* LOWERENDPOINT_H_ */
