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

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
//#include "palLed.h"
#include "palId.h"
#include "palLocalTime.h"
#include "SerialComm.h"
#include "Endpoint.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

#define RATE	20

//#include <util/delay.h>
#include <unistd.h>


class Traffic: public cometos::Endpoint {
public:
	static const int NUMC = 100;

	virtual void initialize() {
		schedule(&myMsg, &Traffic::traffic);
		start = palLocalTime_get();
//		schedule(new Message, &Traffic::delay);
		consistencyFail = 0;
		confirmed = 0;
		sent = 0;
		rcvd = 0;
	}

	void delay(Message* timer) {
		int a = rand() % 30;
		//palLed_toggle(1);
		for (int i = 0; i < a; i++) {
			//_delay_ms(1);
			usleep(1000);
		}
		schedule(timer, &Traffic::delay, rand() % 1000);
	}

	void traffic(Message* timer) {
		sent++;

		DataRequest *request = new DataRequest(BROADCAST, new Airframe,
				createCallback(&Traffic::response));

		for (uint8_t i = 48; i < 48+NUMC; i++) {
			request->getAirframe() << ((uint8_t) i);
		}

		request->getAirframe() << sent;
		send(request);
		if (palId_id() == BASESTATION_ADDR) {
			if (sent % BASESTATION_ADDR == 0) {
				printf("s=%5d|confirmed=%4d\n", sent, confirmed);
			}
		} else {
			//palLed_toggle(1);
		}
//		schedule(timer, &Traffic::traffic, RATE);
	}

	void handleIndication(DataIndication* msg) {
		uint32_t remoteCounter;
		rcvd++;

		msg->getAirframe() >> remoteCounter;
		bool correct = true;
		for (uint8_t i = 48+NUMC-1; i >= 48; i--) {
			uint8_t tmp;
			msg->getAirframe() >> tmp;
//			printf("%3d|", tmp);
			if (tmp != i) {
				correct = false;
			}
		}
//		printf("\n");
		if (!correct) {consistencyFail++; }



		if (palId_id() == BASESTATION_ADDR) {
			if (rcvd % BASESTATION_ADDR == 0) {
				printf("r=%5d|crcFail=%4d\n", rcvd, consistencyFail);
			}
		} else {
//			palLed_toggle(4);
		}

		delete msg;
	}

	void response(DataResponse *resp) {
		end = palLocalTime_get();
		if (resp->success == false) {
		} else {
			confirmed++;
		}
		if (end - start > 10000) {
			printf("Done; start=%d|end=%d|sent=%d|conf=%d|datarate=%.2f|rcvd=%d|rFail=%d\n", start, end, sent, confirmed, (NUMC + 4) * 8 * sent * 1000.0 / (end-start), rcvd, consistencyFail);
			exit(0);
		} else {
			schedule(&myMsg, &Traffic::traffic);
		}
		delete resp;
	}
private:
	uint32_t consistencyFail;
	uint32_t confirmed;
	uint32_t sent;
	uint32_t rcvd;
	uint32_t start;
	uint32_t end;
	cometos::Message myMsg;
} traffic;

int main(int argc, const char *argv[]) {

	SerialComm comm(argv[1]);

	traffic.gateReqOut.connectTo(comm.gateReqIn);
	comm.gateIndOut.connectTo(traffic.gateIndIn);

	cometos::initialize();
	cometos::run();
	return 0;
}

