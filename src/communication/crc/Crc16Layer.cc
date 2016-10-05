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
 * Crc16 Module
 *
 * @author Stefan Unterschuetz
 */

#include "Crc16Layer.h"
#include "crc16.h"
#include "logging.h"

using namespace cometos;

static uint16_t getCrc(uint8_t* array, pktSize_t length, uint16_t initial_crc) {
	uint16_t crc = initial_crc;

	for (pktSize_t i = 0; i < length; i++) {
		crc = crc16_update(crc, array[i]);
	}
	return crc;
}

void Crc16Layer::handleRequest(DataRequest* msg) {
	uint8_t* data = msg->getAirframe().getData();
	pktSize_t length = msg->getAirframe().getLength();
	uint16_t crc = getCrc(data, length, 0xFFFF);
	msg->getAirframe() << crc;
	sendRequest(msg);
}

void Crc16Layer::handleIndication(DataIndication* msg) {
	if (msg->getAirframe().getLength() < 2) {
		LOG_WARN("crc error, message to short");
		delete msg;
		return;
	}
	uint16_t crc;
	msg->getAirframe() >> crc;

	uint8_t* data = msg->getAirframe().getData();
	pktSize_t length = msg->getAirframe().getLength();
	if (crc != getCrc(data, length, 0xFFFF)) {
		LOG_WARN("crc error, checksum doen's match\n");
		delete msg;
		return;
	}

	sendIndication(msg);
}

