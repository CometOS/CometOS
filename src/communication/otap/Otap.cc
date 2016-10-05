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

#include "Otap.h"
#include "OutputStream.h"
namespace cometos {

Define_Module(cometos::Otap);

// debugging function to connect getCout() to C-code
//static void printNums(const char * pre, uint32_t a, uint32_t b) {
//    getCout() << pre << a << "|" << b << cometos::endl;
//}


Otap::~Otap() {
}

Otap::Otap() :
		OtapBase(OtapBase::OTAP_MODULE_NAME) {
}

void Otap::initialize() {
    OtapBase::initialize();

	remoteDeclare(&Otap::initiate, "init");
	remoteDeclare(&Otap::verify, "veri");
}

uint8_t Otap::initiate(OtapInitMessage & msg) {
    palFirmware_ret_t ret = checkInit(msg.slot, msg.segCount);

    if (ret != PAL_FIRMWARE_SUCCESS) {
        return ret;
    }

	ret = palFirmware_erase(msg.slot);
	if (ret != PAL_FIRMWARE_SUCCESS) {
		return ret;
	}

	setSegCount(msg.segCount);
	setCurrSlot(msg.slot);
	getReceivedSegments().fill(false);

	return PAL_FIRMWARE_SUCCESS;
}

void Otap::recvSegment(uint8_t * data, uint16_t segId) {
	if (getCurrSlot() == OtapBase::NO_CURR_SLOT) {
		return;
	}

	palFirmware_write(data, getCurrSlot(), segId);

	getReceivedSegments().set(segId, true);
}



uint8_t Otap::verify(uint16_t& crc, uint32_t &address_offset) {
//    getCout() << "crcHex=" << crc << cometos::endl;
	if (getCurrSlot() == OtapBase::NO_CURR_SLOT) {
		return PAL_FIRMWARE_ERROR;
	}

	// always stop current session
	uint8_t slot = getCurrSlot();
	setCurrSlot(OtapBase::NO_CURR_SLOT);

	if (getReceivedSegments().count(true) < getSegCount()) {
		return PAL_FIRMWARE_ERROR;
	}

	// calculate CRC for the remaining firmware image (should be 0xFF after erase)
	uint16_t remaining = palFirmware_getSlotSize() - getSegCount();
	for (uint16_t i = 0; i < remaining; i++) {
		for (uint16_t j = 0; j < P_FIRMWARE_SEGMENT_SIZE; j++) {
			crc = palFirmware_crc_update(crc, 0xFF);
		}
//		getCout() << "crcSlotRest=" << crc << cometos::endl;
	}
//	getCout() << "crcSlot=" << crc << cometos::endl;
	return palFirmware_validate(slot, address_offset, crc);
}


}

