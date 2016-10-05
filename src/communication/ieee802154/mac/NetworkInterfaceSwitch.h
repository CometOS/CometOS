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

#ifndef NETWORK_INTERFACE_SWITCH_H_
#define NETWORK_INTERFACE_SWITCH_H_

#include "types.h"
#include "LowerEndpoint.h"
#include "OverwriteAddrData.h"

/**
 * This protocol allows the redirection of packets to a serial interface.
 * Currently the class is used for coupling wireless transmission and
 * serial transmission. The functionality is transparent to upper layers.
 */
class NetworkInterfaceSwitch : public cometos::LowerEndpoint  {
public:

	NetworkInterfaceSwitch(const char* name=NULL,node_t cniAddrBegin=0x0000,node_t cniAddrEnd=0x1000);

	virtual void handleIndication(cometos::DataIndication* msg);

	virtual void initialize();

	virtual void handleRequest(cometos::DataRequest* msg);


	// Common Network Interface Gates
	cometos::InputGate<cometos::DataIndication>  cniIndIn;
	cometos::OutputGate<cometos::DataRequest> cniReqOut;

	// Network-to-Network Interface Gates
	cometos::InputGate<cometos::DataIndication>  nniIndIn;
	cometos::OutputGate<cometos::DataRequest> nniReqOut;

private:
	OverwriteAddrData * createAddrMetaData(const cometos::DataRequest * const msg);

	node_t cniAddrBegin;
	node_t cniAddrEnd;

};

#endif /*NETWORK_INTERFACE_SWITCH_H_ */
