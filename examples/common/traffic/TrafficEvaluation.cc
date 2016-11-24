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
 * @author Florian Kauer
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "TrafficEvaluation.h"
#include "Airframe.h"
#include "NetworkTime.h"
#include "palLed.h"
#include "OutputStream.h"
#include "logging.h"
#include "MacAbstractionBase.h"
#include "palFirmware.h"
#include <math.h>
#include "palId.h"

/*METHOD DEFINITION----------------------------------------------------------*/


namespace cometos {

Define_Module(TrafficEvaluation);

TrafficEvaluation::TrafficEvaluation(uint8_t msgSize,
                               timeOffset_t meanInterval) :
        Endpoint("traf"),
        destinationSet(false),
        meanInterval(meanInterval),
        frame(NULL),
        myCrc(0xFFFF),
        msgSize(msgSize),
        sequenceNumber(0)
{}

void TrafficEvaluation::initialize() {
	Endpoint::initialize();

#ifdef OMNETPP
    msgSize = par("msgSize");
	meanInterval = par("meanInterval");
	if(par("destination").operator int() != -1) {
	    destination = par("destination");
	    destinationSet = true;
	}
#endif

	ASSERT(msgSize + sizeof(myCrc) + sizeof(sequenceNumber) <= AIRFRAME_MAX_SIZE);
	schedule(new Message, &TrafficEvaluation::traffic, 0);
	counter = 0;
	failed = 0;
	frame = new Airframe();
	for (uint8_t i = 0; i < msgSize - sizeof(myCrc) - sizeof(sequenceNumber) - 1 /*dummy*/; i++) {
	    uint8_t data = intrand(256);
	    myCrc = palFirmware_crc_update(myCrc, data);
	    (*frame) << data;
	}
	(*frame) << myCrc;
}

void TrafficEvaluation::finish() {
    recordScalar("sent", counter);
}

void TrafficEvaluation::traffic(Message *timer) {
    double rnd = rand()/(double)RAND_MAX;
    double x = -log(1-rnd)*meanInterval; // interval = 1/lambda
    schedule(new Message, &TrafficEvaluation::traffic, x);

    if(!destinationSet) {
        delete(timer);
        return;
    }

	Airframe *msg = frame->getCopy();

    uint8_t dummy = 0;

	sequenceNumber++;
	(*msg) << sequenceNumber;
    (*msg) << dummy;

	//LOG_INFO("tx:    dst=0x" << cometos::hex << destination << "|seq=" << cometos::dec << counter << "|failed=" << failed);
	LOG_INFO("!0x" << hex << palId_id() << "!T!" << (dummy?"d":"m") << "!" << dec << sequenceNumber);
	LOG_INFO("dst=0x" << hex << destination << "|attempts=" << dec << counter << "|failed=" << failed);

	ts = NetworkTime::get();
	sendRequest(
			new DataRequest(destination, msg,
					createCallback(&TrafficEvaluation::resp)));

	delete timer;
};

void TrafficEvaluation::resp(DataResponse *response) {
	//LOG_INFO("finish transmission: " << response->success);

	if(!response->success) {
		failed++;
	}

	delete response;
	counter++;
	palLed_toggle(1);
}

void TrafficEvaluation::handleIndication(DataIndication* msg) {
    int64_t remoteSequenceNumber;
    uint8_t dummy;
    msg->getAirframe() >> dummy;
    msg->getAirframe() >> remoteSequenceNumber;

	uint16_t crc = 0xFFFF;
	uint16_t sentCrc;
	msg->getAirframe() >> sentCrc;
	uint8_t * data = msg->getAirframe().getData();
	if (msg->getAirframe().getLength() == msgSize - sizeof(myCrc) - sizeof(sequenceNumber) - 1 /*dummy*/) {
        for (uint8_t i = 0; i < msgSize + 2 - sizeof(myCrc) - sizeof(sequenceNumber) - 1 /*dummy*/; i++) {
            crc = palFirmware_crc_update(crc, data[i]);
        }
        crc = palFirmware_crc_update(crc, (uint8_t) crc >> 8);
        crc = palFirmware_crc_update(crc, (uint8_t) crc & 0xFF);
	}
	if (crc != 0) {
	    palLed_toggle(4);
	    LOG_WARN("corrupted frame received");
	} else {
        int16_t rssi = RSSI_INVALID;
        if (msg->has<MacRxInfo>()) {
            rssi = msg->get<MacRxInfo>()->rssi;
            (void) rssi; // avoid warning if logging is disabled
        }

	    LOG_INFO("!0x" << hex << palId_id() << "!R!" << (dummy?"d":"m") << "!" << dec << sequenceNumber);
        LOG_INFO("dst=0x" << hex << msg->dst << "|src=0x" << msg->src << dec << "|RSSI=" << rssi);

        palLed_toggle(2);
	}
	delete msg;
}

void TrafficEvaluation::setDestination(node_t destination) {
    destinationSet = true;
    this->destination = destination;
}

}// namespace cometos
