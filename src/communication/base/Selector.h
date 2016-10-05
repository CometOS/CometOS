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

#ifndef SELECTOR_H_
#define SELECTOR_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "OutputStream.h"
#include "logging.h"
#include "LowerEndpoint.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

template<uint8_t N>
class Selector: public LowerEndpoint {
public:
	Selector() :
			gateIndIn(this, &Selector::handleIndication, "gateIndIn"), gateReqOut(
					this, "gateReqOut"), sel(0) {
	}

	virtual void handleRequest(DataRequest* msg) {
		LOG_INFO("Send DtReq to "<<sel);
		gateReqOut.send(sel, msg);
	}

	virtual void handleIndication(DataIndication* msg) {
		LOG_INFO("Send DtInd");
		sendIndication(msg);
	}

	void setSel(uint8_t sel) {
		ASSERT(sel<N);
		this->sel = sel;
	}

	InputGateArray<DataIndication, N> gateIndIn;
	OutputGateArray<DataRequest, N> gateReqOut;

protected:
	uint8_t sel;

};

} /* namespace cometos */
#endif /* SELECTOR_H_ */
