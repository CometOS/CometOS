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

#ifndef OTAPBLOCKTRANSFER_H_
#define OTAPBLOCKTRANSFER_H_

#include <cometos.h>
#include <Endpoint.h>

#include "BitVector.h"
#include "ReedSolomonCoding.h"


/**in milliseconds*/
#ifndef OTAP_DEFAULT_PACKET_INTERVAL
#define OTAP_DEFAULT_PACKET_INTERVAL	150
#endif
namespace cometos {

/**Contains code for OTAP data dissemination.
 *
 * TODO currently segments are not allowed to overlap during
 * transmission since performance is highly affected by this.
 *
 * TODO History and duplicate filter
 *
 */
class OtapBlockTransferBase: public Endpoint {
public:
	OtapBlockTransferBase(const char * service_name);

	virtual void initialize();

	virtual void handleIndication(DataIndication* msg);

	virtual void recvSegment(uint8_t * data, uint16_t segId);

	void setIntervalRemote(uint16_t & interval);

	void setInterval(uint16_t interval);

	/**@return number of packets of one segment
	 */
	virtual uint8_t getNumPkts() =0;

	/**@return number of redundant packets of one packet
	 */
	virtual uint8_t getRedPkts() =0;

	/**@return size of a packet*/
	virtual uint8_t getPktSize() =0;

	virtual bool tryDecoding() = 0;
	virtual void encode() = 0;

	virtual void clearRxHist() = 0;
	virtual void setRxHist(uint8_t num) = 0;

protected:

	void initDataArrays();

	uint8_t **pkt;
	uint8_t *pktArray;
	uint8_t **red;
	uint8_t *redArray;
	timeOffset_t interval;

private:

	uint16_t curSegment;bool curProcessed;
	uint8_t isSending;

#ifdef OTAP_SINK_CODE
public:

	/**Method is thread-safe*/
	bool sendSegment(node_t dst, uint8_t * data, uint16_t segId);

	virtual void sendDone(uint16_t segId);

	bool sendNext(bool thread_safe);
	void response(DataResponse* resp);

	node_t dst;
#endif
};

template<uint8_t NUM_PKTS, uint8_t NUM_RED_PKTS, uint8_t PKT_SIZE>
class OtapBlockTransfer: public OtapBlockTransferBase {
public:
	virtual ~OtapBlockTransfer() {

	}

	OtapBlockTransfer(const char * service_name) :
			OtapBlockTransferBase(service_name) {
		pkt = pkt_;
		pktArray = pktArray_;
		red = red_;
		redArray = redArray_;

		initDataArrays();
	}

	uint8_t getNumPkts() {
		return NUM_PKTS;
	}

	uint8_t getRedPkts() {
		return NUM_RED_PKTS;
	}

	uint8_t getPktSize() {
		return PKT_SIZE;
	}

	bool tryDecoding() {
		return coding.streamDecoding(pkt, recvPkt, red, recvRed, PKT_SIZE);
	}

	void encode() {
		coding.streamEncoding(pkt, red, PKT_SIZE);
	}

	void clearRxHist() {
		recvPkt.fill(false);
		recvRed.fill(false);
	}

	void setRxHist(uint8_t num) {
		if (num < NUM_PKTS) {
			recvPkt.set(num, true);
		} else {
			recvRed.set(num - NUM_PKTS, true);
		}
	}


private:

	uint8_t *pkt_[NUM_PKTS];
	uint8_t pktArray_[NUM_PKTS * PKT_SIZE];
	uint8_t *red_[NUM_RED_PKTS];
	uint8_t redArray_[NUM_RED_PKTS* PKT_SIZE];

	cometos::BitVector<NUM_PKTS> recvPkt;
	cometos::BitVector<NUM_RED_PKTS> recvRed;

	ReedSolomonCoding<NUM_PKTS, NUM_RED_PKTS> coding;

};

}

#endif /* OTAPBLOCKTRANSFER_H_ */
