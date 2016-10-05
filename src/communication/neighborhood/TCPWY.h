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
 * @author Gerry Siegemund
 */

#ifndef TCPWY_H_
#define TCPWY_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Layer.h"
#include "TCPWYHeader.h"
#include "TCPWYControlHeader.h"
#include "TZTCAlgo.h"



/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class TCPWY : public cometos::Layer {
public:
    TCPWY();

	/**Sets parameters*/
    void initialize();
//    void finish();

	void neighborDataUpdateTimer(Message *timer);
	void timedReport(Message *timer);
	void resp(DataResponse *response);

	virtual void handleIndication(DataIndication* msg);
	virtual void handleRequest(DataRequest* msg);

	void sendControl(uint8_t controlType, node_t targetId);
	void handleControl(DataRequest* msg);

    TZTCAlgo tca;

	InputGate<DataRequest>  pGateControlIn;
	OutputGate<DataIndication>  pGateControlOut;


private:
    void setHeaderData(TCPWYHeader &header);
    bool checkMsgType(Airframe& msg);


	uint32_t ts;

	cometos::Airframe * frame;

	uint8_t seqNum;

	bool mHasUpperLayer; // bool to indicate whether there are layers above TCA
	bool mSendTCAHeader; // bool to indicate whether next send msg should have a TCA header
	bool mHasSend; // indicater whether this node has send a msg within the last period
	bool toggle; // toggle for check deletion and stuff

};

} // namespace cometos

#endif /* TCPWY_H_ */
