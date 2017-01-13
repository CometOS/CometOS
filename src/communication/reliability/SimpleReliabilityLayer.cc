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
 * @author Stefan Unterschuetz, Andreas Weigel
 */

#include "SimpleReliabilityLayer.h"
#include "logging.h"

namespace cometos {

Define_Module( SimpleReliabilityLayer);

SimpleReliabilityLayer::SimpleReliabilityLayer(const char * service_name) :
        cometos::Layer(service_name),
        retryCounter(0),
        retransmissionCfg(SRL_DEFAULT_ACK_TO | SRL_DEFAULT_NUM_RETRIES),
        current(NULL)
{
    ackTimeoutTimer = new Message();
    refreshTimer = new Message();
}

SimpleReliabilityLayer::~SimpleReliabilityLayer()
{
    CANCEL_AND_DELETE(ackTimeoutTimer);
    CANCEL_AND_DELETE(refreshTimer);
}

void SimpleReliabilityLayer::initialize() {
	Layer::initialize();
	ack.counter = 0;
	ack.direction = 0;
	schedule(refreshTimer, &SimpleReliabilityLayer::refreshHist, RELIABILITY_TIME_TO_LIVE);

	remoteDeclare(&SimpleReliabilityLayer::setCfg, "setCfg");
	remoteDeclare(&SimpleReliabilityLayer::getCfg, "getCfg");
}

void SimpleReliabilityLayer::finish() {
	if (current != NULL)
		delete current;
}

void SimpleReliabilityLayer::handleRequest(cometos::DataRequest *msg) {

    if (msg->dst == MAC_BROADCAST) {
        msg->response(new cometos::DataResponse(DataResponseStatus::INVALID_ADDRESS));
        delete(msg);
        return;
    }

	// in case of a new request, give up on current request
	if (current != NULL) {
		LOG_WARN("DISCRD pckt");
		delete(current);
	}

    ack.counter++;
    msg->getAirframe() << ack;
    current = msg;
    retryCounter = SRL_GET_NUM_RETRIES(retransmissionCfg);

    sendCopy(current);
}

void SimpleReliabilityLayer::handleIndication(cometos::DataIndication *msg) {
//    std::cout << "received msg of len " << (int) msg->getAirframe().getLength();
	SimpleReliabilityHeader rack;
	if (msg->getAirframe().getLength() < SimpleReliabilityHeader::getSerializedLen()) {
	    delete(msg);
	    return;
	}
    msg->getAirframe() >> rack;

//    std::cout << " " << (int)rack.counter << "|" << (int)rack.direction << endl;
    LOG_DEBUG("RCVD pckt dst=" << msg->dst << " src="
					   << msg->src << " isAck=" << (int) rack.direction
					   << " ackCnt=" << (int) ack.counter << " rackCnt=" << (int) rack.counter);
    if (0 == rack.direction) {
//        std::cout << "data " << endl;
        // send ack back
        cometos::DataRequest *ackPkt = new cometos::DataRequest(msg->src, cometos::make_checked<cometos::Airframe>());
        rack.direction = 1;

        ackPkt->getAirframe() << rack;
        sendRequest(ackPkt);

        // check if message was already forwarded
        if (history.checkAndAdd(MessageId(msg->src, rack.counter))) {
            delete msg;
        } else {
            // send data to upper layer
            sendIndication(msg);
        }
    } else if (current != NULL) {
//        std::cout << "ack myCount=" << ack.counter << endl;
        // check if ack packet matches current packet
        if (rack.counter == ack.counter) {
            cancel(ackTimeoutTimer);
            current->response(new cometos::DataResponse(DataResponseStatus::SUCCESS));
            LOG_DEBUG("ACK for pckt w/ id " << (uintptr_t) current->getRequestId());
            delete current;
            current = NULL;
        }
        delete msg;
    } else {
//        std::cout << "invalid " << endl;
        delete msg;
    }
}

void SimpleReliabilityLayer::handleConfirm(cometos::DataResponse * msg) {
    delete(msg); // TODO maybe good idea to start timer here
}


bool SimpleReliabilityLayer::setCfg(SrlConfig & cfg) {
	// TODO add mechanism to schedule setting
	if (cfg.numRetries > 0 && cfg.numRetries <= SRL_RETRY_MASK) {
		retransmissionCfg = cfg.numRetries;
	} else {
		return false;
	}
	retransmissionCfg |= (cfg.ackTimeout & SRL_ACK_TO_MASK);
	return true;
}

SrlConfig SimpleReliabilityLayer::getCfg() {
	SrlConfig config(retransmissionCfg & SRL_ACK_TO_MASK,
			         retransmissionCfg & SRL_RETRY_MASK);
	return config;
}


void SimpleReliabilityLayer::refreshHist(cometos::Message * timer) {
    history.refresh();
    schedule(refreshTimer, &SimpleReliabilityLayer::refreshHist, RELIABILITY_TIME_TO_LIVE);
}

void SimpleReliabilityLayer::timeout(cometos::Message *timer) {
	ASSERT(timer == ackTimeoutTimer);
	ASSERT(current != NULL);

	LOG_WARN("TIMEOUT,retryCntr=" << (int) retryCounter << " currPcktId=" << (uintptr_t) current->getRequestId());
	if (retryCounter == 0) {
		current->response(new cometos::DataResponse(DataResponseStatus::NO_ACK));
		delete current;
		current = NULL;
	} else {
		retryCounter--;
		sendCopy(current);
	}
}

void SimpleReliabilityLayer::sendCopy(cometos::DataRequest* & msg) {
    cometos::DataRequest * req =
            new cometos::DataRequest(msg->dst, cometos::AirframePtr(msg->getAirframe().getCopy()),
                                     createCallback(&SimpleReliabilityLayer::handleConfirm));
//    std::cout << "send message of len " << (int) req->getAirframe().getLength() << endl;
    // TODO metadata is lost here
    sendRequest(req);
    reschedule(ackTimeoutTimer, &SimpleReliabilityLayer::timeout, SRL_GET_ACK_TO(retransmissionCfg));
}

}


namespace cometos {
    void serialize(ByteVector & buf, const SimpleReliabilityHeader & value) {
        uint8_t byte = (value.direction << 7) | value.counter;
        serialize(buf, byte);
    }

    void unserialize(ByteVector & buf, SimpleReliabilityHeader & value) {
        uint8_t byte;
        unserialize(buf, byte);
        value.counter = byte & 0x7F;
        value.direction = (byte >> 7) & 0x01;
    }

    void serialize(ByteVector & buf, SrlConfig const & value) {
    	uint16_t cfg;
    	cfg = value.ackTimeout & SRL_ACK_TO_MASK;
    	cfg |= value.numRetries & SRL_RETRY_MASK;
    	serialize(buf, cfg);
    	serialize(buf, value.when);
    }

	void unserialize(ByteVector & buf, SrlConfig & value) {
		uint16_t cfg;
		unserialize(buf, value.when);
		unserialize(buf, cfg);
		value.ackTimeout = cfg & SRL_ACK_TO_MASK;
		value.numRetries = cfg & SRL_RETRY_MASK;
	}
}
