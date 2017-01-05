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

/*INCLUDES-------------------------------------------------------------------*/

#include "TrafficExample.h"
#include "Airframe.h"
#include "NetworkTime.h"
#include "palLed.h"
#include "OutputStream.h"
#include "logging.h"
#include "MacAbstractionBase.h"
#include "palFirmware.h"

#ifdef OMNETPP
#include <sstream>
#include <iomanip>
#endif

/*METHOD DEFINITION----------------------------------------------------------*/


namespace cometos {

Define_Module(TrafficExample);

TrafficExample::TrafficExample(StaticSList<node_t, 10> destAddresses,
                               uint8_t msgSize,
                               timeOffset_t fixedInterval,
                               timeOffset_t rndInterval,
                               bool snoop) :
        Endpoint("traf"),
        gateSnoopIn(this, &TrafficExample::handleSnoopIndication, "gateSnoopIn"),
        fI(fixedInterval),
        rI(rndInterval),
        destAddresses(destAddresses),
        frame(NULL),
        myCrc(0xFFFF),
        msgSize(msgSize),
        snoop(snoop),
        sequenceNumber(0)
{}

void TrafficExample::initialize() {
	Endpoint::initialize();

#ifdef OMNETPP
    msgSize = par("msgSize");
	fI = par("fixedInterval");
	rI = par("rndInterval");
#endif

	ASSERT(msgSize + sizeof(myCrc) + sizeof(sequenceNumber) <= AIRFRAME_MAX_SIZE);
	schedule(new Message, &TrafficExample::traffic, intrand(rI) + fI);
	counter = 0;
	failed = 0;
	frame = new Airframe();
	for (uint8_t i = 0; i < msgSize - sizeof(myCrc) - sizeof(sequenceNumber); i++) {
	    uint8_t data = intrand(256);
	    myCrc = palFirmware_crc_update(myCrc, data);
	    (*frame) << data;
	}
	(*frame) << myCrc;

	CONFIG_NED(snoop);

	if (snoop) {
	    MacAbstractionBase* mac = static_cast<MacAbstractionBase*>(getModule("mac"));
	    if (mac!=NULL) {
	        mac->setPromiscuousMode(snoop);
	    }
	}
	// in sim, we set destinations by using a whitespace-separated string
#ifdef OMNETPP
    timeLimitMS = par("timeLimitMS");

	const char *dist = par("destinations");
	omnetpp::cStringTokenizer tokenizer(dist);
    while (tokenizer.hasMoreTokens())
        destAddresses.push_back(atoi(tokenizer.nextToken()));
    LOG_INFO("Initialized with " << (int) destAddresses.size() << " destinations");
#endif
}

void TrafficExample::finish() {
    recordScalar("sent", counter);

#ifdef OMNETPP
    for(auto rec : receptions) {
        std::stringstream ss;
        ss << "received_corrupted_" << std::setfill('0') << std::setw(3) << rec.first;
        recordScalar(ss.str().c_str(), rec.second.corrupted);
        std::stringstream ss2;
        ss2 << "received_unique_" << std::setfill('0') << std::setw(3) << rec.first;
        recordScalar(ss2.str().c_str(), rec.second.uniques);
        std::stringstream ss3;
        ss3 << "received_duplicate_" << std::setfill('0') << std::setw(3) << rec.first;
        recordScalar(ss3.str().c_str(), rec.second.duplicates);
    }
#endif
}

void TrafficExample::traffic(Message *timer) {
#ifdef OMNETPP
    if(timeLimitMS > 0 && omnetpp::simTime().inUnit(omnetpp::SIMTIME_MS) >= timeLimitMS) {
        delete(timer);
        return;
    }
#endif

    schedule(new Message, &TrafficExample::traffic, intrand(rI) + fI);

    if(destAddresses.size() == 0) {
        delete(timer);
        return;
    }

	Airframe *msg = frame->getCopy();

	sequenceNumber++;
	(*msg) << sequenceNumber;

	node_t dst = destAddresses[intrand(destAddresses.size())];
	LOG_INFO("tx:    dst=0x" << cometos::hex << dst << "|seq=" << cometos::dec << counter << "|failed=" << failed);

	ts = NetworkTime::get();
	sendRequest(
			new DataRequest(dst, msg,
					createCallback(&TrafficExample::resp)));

	delete timer;
};

void TrafficExample::resp(DataResponse *response) {
	LOG_INFO("finish transmission: " << response->str());

	if(response->isFailed()) {
		failed++;
	}

	delete response;
	counter++;
	palLed_toggle(1);
}

void TrafficExample::handleIndication(DataIndication* msg) {
#ifdef OMNETPP
    auto target = receptions.find(msg->src);
    if(target == receptions.end()) {
        receptions[msg->src] = Reception();
        target = receptions.find(msg->src);
    }
#endif

    int64_t remoteSequenceNumber;
    msg->getAirframe() >> remoteSequenceNumber;

	uint16_t crc = 0xFFFF;
	uint16_t sentCrc;
	msg->getAirframe() >> sentCrc;
	uint8_t * data = msg->getAirframe().getData();
	if (msg->getAirframe().getLength() != msgSize - sizeof(myCrc) - sizeof(sequenceNumber) - 1 /*type*/) {
	    palLed_toggle(4);
	    LOG_WARN("too short frame received");
        delete msg;
        return;
    }
    
    for (uint8_t i = 0; i < msgSize - sizeof(myCrc) - sizeof(sequenceNumber) - 1 /*type*/; i++) {
        crc = palFirmware_crc_update(crc, data[i]);
	}

	if (crc != sentCrc) {
	    palLed_toggle(4);
#ifdef OMNETPP
	    target->second.corrupted++;
#endif
	    LOG_WARN("corrupted frame received");
	} else {
        int16_t rssi = RSSI_INVALID;
        if (msg->has<MacRxInfo>()) {
            rssi = msg->get<MacRxInfo>()->rssi;
            (void) rssi; // TODO use?
        }
        LOG_INFO("rx:    dst=0x" << hex << msg->dst << "|src=0x" << msg->src << dec << "|RSSI=" << rssi);

#ifdef OMNETPP
        if(remoteSequenceNumber > target->second.lastSeqNum) {
            target->second.uniques++;
            target->second.lastSeqNum = remoteSequenceNumber;
        }
        else {
            target->second.duplicates++;
        }
#endif

        palLed_toggle(2);
	}
	delete msg;
}

void TrafficExample::handleSnoopIndication(DataIndication * msg) {
    LOG_INFO("snoop: dst=0x" << hex << msg->src << "|src=0x" << msg->dst);
    delete(msg);
}

}// namespace cometos
