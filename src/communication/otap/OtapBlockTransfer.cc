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

#include "OtapBlockTransfer.h"

#ifdef OTAP_SINK_CODE
#include "palExec.h"
#endif

using namespace cometos;


OtapBlockTransferBase::OtapBlockTransferBase(const char * service_name) :
		Endpoint(service_name),
		interval(OTAP_DEFAULT_PACKET_INTERVAL),
		curSegment(-1),
		curProcessed(false),
		isSending(0)
{
}

void OtapBlockTransferBase::initialize() {
    Endpoint::initialize();
}

void OtapBlockTransferBase::initDataArrays() {
	// prepare buffers
	uint16_t pktSize=getPktSize();
	for (uint8_t i = 0; i < getNumPkts(); i++) {
		pkt[i] = &pktArray[  pktSize * i];
	}
	for (uint8_t i = 0; i <getRedPkts(); i++) {
		red[i] = &redArray[ pktSize * i];
	}
}

// OTAP packet format
// SEG_NUM | PKT_NUM | 64 Byte Data
void OtapBlockTransferBase::handleIndication(DataIndication* msg) {

	uint16_t segment;
	uint8_t num;

	if (isSending > 0) {
		delete msg;
		return;
	}

	msg->getAirframe() >> segment >> num;

	// message of invalid length
	if (msg->getAirframe().getLength() != getPktSize()) {
		ASSERT(false);
		delete msg;
		return;
	}

	// start receiving new segment
	if (curSegment != segment) {
		clearRxHist();

		curSegment = segment;
		curProcessed = false;
	}

	// already processed segment
	if (curProcessed) {
		delete msg;
		return;
	}

	// save packets in list
	if (num < getNumPkts()) {
		memcpy(pkt[num], msg->getAirframe().getData(),getPktSize());
		setRxHist(num);
	} else if (num < getNumPkts() +  getRedPkts()) {
		memcpy(red[num-getNumPkts()], msg->getAirframe().getData(),getPktSize());
		setRxHist(num);
	} else {
		ASSERT(false);
	}
	delete msg;

	// try decoding, if succesful, then store data
	if (tryDecoding()) {
		// write page
		curProcessed = true;
		recvSegment(pktArray, segment);
	}
}

void OtapBlockTransferBase::recvSegment(uint8_t * data, uint16_t segnum) {

}

void OtapBlockTransferBase::setIntervalRemote(uint16_t & interval) {
	setInterval(interval);
}

void OtapBlockTransferBase::setInterval(uint16_t interval) {
    this->interval = interval;
}

#ifdef OTAP_SINK_CODE

bool OtapBlockTransferBase::sendSegment(node_t dst, uint8_t * data, uint16_t segment) {

	if (isSending > 0) {
		return false;
	}

	this->dst = dst;

	// copy segment to internal array
	uint16_t pktSize = getPktSize();
	for (uint8_t i = 0; i < getNumPkts(); i++) {
		memcpy(pkt[i], data + i * pktSize, pktSize);
	}
	encode();

	curSegment = segment;
	sendNext(true);
	return true;
}

bool OtapBlockTransferBase::sendNext(bool thread_safe) {

	if (thread_safe) {
		palExec_atomicBegin();
	}
	if (isSending >= getNumPkts() + getRedPkts()) {
		return false;
	}
	Airframe* msg = new Airframe;
	msg->setLength(getPktSize());

	if (isSending < getNumPkts()) {
		memcpy(msg->getData(), pkt[isSending], getPktSize());
	} else if (isSending < getNumPkts() + getRedPkts()) {
		memcpy(msg->getData(), red[isSending - getNumPkts()], getPktSize());
	} else {
		ASSERT(false);
	}
	(*msg) << isSending << curSegment;

	isSending++;

	DataRequest* request=new DataRequest(dst, msg,
			createCallback(&OtapBlockTransferBase::response));

	if (thread_safe) {
		palExec_atomicEnd();
	}

	sendRequest(request, interval);

	return true;
}

void OtapBlockTransferBase::sendDone(uint16_t segnum) {}

void OtapBlockTransferBase::response(DataResponse* resp) {
    delete resp;
	if (sendNext(false) == false) {
		isSending = 0;
		sendDone(curSegment);
	}
}

#endif

