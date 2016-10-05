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

#ifndef SIMPLE_RELIABILITY_LAYER_H_
#define SIMPLE_RELIABILITY_LAYER_H_

#include "Layer.h"
#include "DuplicateFilter.h"
#ifdef PAL_MAC
#include "mac_definitions.h"
#else
#define MAC_BROADCAST 0xFFFF
#endif

#define RELIABILITY_MODULE_NAME "srl"
#define SRL_DEFAULT_ACK_TO	1024
#define SRL_DEFAULT_NUM_RETRIES	3


#define SRL_RETRY_MASK (0xF)
#define SRL_ACK_TO_MASK (0xFFF0)
// timeout in ms is taken from most significant 12 bits of cfg
#define SRL_GET_ACK_TO(cfg)      ((cfg) & SRL_ACK_TO_MASK)
// number of retries encoded least significant 4 bits of cfg
#define SRL_GET_NUM_RETRIES(cfg) ((cfg) & SRL_RETRY_MASK)


#define RELIABILITY_HISTORY_SIZE	20
#define RELIABILITY_TIME_TO_LIVE	5000

namespace cometos {

/**
 * Timeout and retransmission configuration class
 */
struct SrlConfig {
	/**
	 * @param ackTimeout timeout before retransmission in multiples of 16 ms
	 * @param numRetries number of retries before transmission is given up (0-15)
 	 * @param when       time offset, determines when the new settings will be adopted
	 */
	SrlConfig(timeOffset_t ackTimeout = SRL_DEFAULT_ACK_TO,
			  uint8_t numRetries = SRL_DEFAULT_NUM_RETRIES,
			  time_ms_t when = 0) :
				  ackTimeout(ackTimeout),
				  numRetries(numRetries),
				  when(when)
	{}

	timeOffset_t ackTimeout;
	uint8_t numRetries;
	time_ms_t when;
};

void serialize(ByteVector & buf, SrlConfig const & value);
void unserialize(ByteVector & buf, SrlConfig & value);


struct SimpleReliabilityHeader {
    uint8_t direction :1;
    uint8_t counter :7;

    static cometos::pktSize_t getSerializedLen() {
        return 1;
    }
};

struct MessageId {
    MessageId(uint16_t src = MAC_BROADCAST, uint8_t seq = 0) :
            src(src), seq(seq) {
    }
    node_t src;
    uint8_t seq;

    bool operator==(const MessageId & other) {
        return this->src == other.src && this->seq == other.seq;
    }
};

/**
 * This protocol can be used above a routing protocols, to provide end-to-end
 * reliability.
 * After sending a packet this layer waits a predefined time for an acknowledgment, otherwise
 * the packet is discarded.
 *
 */
class SimpleReliabilityLayer : public cometos::Layer {
public:
    SimpleReliabilityLayer(const char * service_name = NULL);
    ~SimpleReliabilityLayer();

	virtual void initialize();

	virtual void finish();
	virtual void handleRequest(cometos::DataRequest *msg);

	virtual void handleIndication(cometos::DataIndication *msg);

	virtual void handleConfirm(cometos::DataResponse * msg);

	/**
	 * Set the timeout and retransmission parameters of this node
	 *
	 * @param cfg configuration object the
	 *
	 * @return true, if new configuration will be adopted
	 *         false, if an error occurred
	 */
	bool setCfg(SrlConfig & cfg);

	SrlConfig getCfg();

private:
	void timeout(cometos::Message* timer);
	void refreshHist(cometos::Message* timer);
	void sendCopy(cometos::DataRequest* & msg);

	uint8_t retryCounter;

	uint16_t retransmissionCfg;

	cometos::DataRequest* current;
	cometos::Message* ackTimeoutTimer;
	cometos::Message* refreshTimer;
	SimpleReliabilityHeader ack;
	DuplicateFilter<MessageId,RELIABILITY_HISTORY_SIZE> history;
};

void serialize(ByteVector & buf, SimpleReliabilityHeader const & value);
void unserialize(ByteVector & buf, SimpleReliabilityHeader & value);



} 

#endif /* SIMPLE_RELIABILITY_LAYER_H_ */
