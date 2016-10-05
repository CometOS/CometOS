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

#include <iostream>
#include "palFirmware.h"
#include "palExecUtils.h"
#include "firmwareVersion.h"
#include "palFirmwareDefs.h"
#include "logging.h"


using namespace std;


/*Used for testing OTAP*/
static uint8_t slots[P_FIRMWARE_NUM_SLOTS][P_FIRMWARE_SEGMENT_SIZE*P_FIRMWARE_NUM_SEGS_IN_STORAGE] = {{0}};
static bool isValid[P_FIRMWARE_NUM_SLOTS] = {0};
static bool isFinal[P_FIRMWARE_NUM_SLOTS] = {0};

firmwareVersionNumber_t firmware_getVersionNumber() {
    return 0;
}


firmwareTimestamp_t firmware_getTimestamp() {
    return 0;
}

firmwareVersion_t firmware_getVersion() {
    return 0;
}

palFirmware_ret_t palFirmware_init() {
	cout<<"Inititalize firmware update abstraction layer"<<endl;
	for (int i=0; i< P_FIRMWARE_NUM_SLOTS; i++) {

	}
	return PAL_FIRMWARE_SUCCESS;
}




bool palFirmware_isValid(uint8_t slot) {
	if (slot >=P_FIRMWARE_NUM_SLOTS) {
		return false;
	}
	return isValid[slot];
}


bool palFirmware_isFinal(uint8_t slot) {
	if (slot >=P_FIRMWARE_NUM_SLOTS) {
		return true;
	}
	return isFinal[slot];
}

uint8_t palFirmware_getNumSlots() {
	return P_FIRMWARE_NUM_SLOTS;
}


uint16_t palFirmware_getSlotSize() {
	return P_FIRMWARE_NUM_SEGS_IN_STORAGE;
}




palFirmware_ret_t palFirmware_erase(uint8_t slot){
	if (slot >=palFirmware_getNumSlots()) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}
	if (isFinal[slot]) {
		return PAL_FIRMWARE_IS_FINAL;
	}

	cout<<"erase "<<(int)slot<<endl;

	isValid[slot]=false;

	memset ( slots[slot], 0xff,  P_FIRMWARE_SEGMENT_SIZE*P_FIRMWARE_NUM_SEGS_IN_STORAGE);
	return PAL_FIRMWARE_SUCCESS;
}



palFirmware_ret_t palFirmware_write(const uint8_t *data,uint8_t slot, uint16_t segIndex) {
	if (slot >=P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}
	if (segIndex>=P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
		return PAL_FIRMWARE_INVALID_SEGMENT;
	}
	if (isFinal[slot]) {
		return PAL_FIRMWARE_IS_FINAL;
	}

	cout<<"write "<<(int)slot<<" seg "<<segIndex<<endl;
	isValid[slot]=false;
	uint32_t address = P_FIRMWARE_SEGMENT_SIZE;
	address *= segIndex;
	memcpy(slots[slot] + address, data,P_FIRMWARE_SEGMENT_SIZE);
	return PAL_FIRMWARE_SUCCESS;
}


palFirmware_ret_t palFirmware_read(uint8_t *data, uint8_t slot, uint16_t segIndex) {
	if (slot >=P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}
	if (segIndex>=P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
		return PAL_FIRMWARE_INVALID_SEGMENT;
	}
	memcpy(data,slots[slot] +  segIndex * P_FIRMWARE_SEGMENT_SIZE,P_FIRMWARE_SEGMENT_SIZE);
	return PAL_FIRMWARE_SUCCESS;
}


palFirmware_ret_t palFirmware_validate(uint8_t slot, uint32_t address_offset, uint16_t crc) {
	if (slot >=P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}
	if (address_offset!=0) {
		return PAL_FIRMWARE_ADDRESS_ERROR;
	}



	isValid[slot]=false;

	uint16_t crc_ = 0xFFFF;
	uint8_t* p = slots[slot];
	uint32_t memend = P_FIRMWARE_SEGMENT_SIZE;
	memend *= P_FIRMWARE_NUM_SEGS_IN_STORAGE;

	for (uint32_t i=0; i < memend; i++) {
		crc_= palFirmware_crc_update(crc_,*p);
		p++;
	}

	cout<<"CRC "<<crc<<"  CRC CALC "<<crc_<<endl;

	if (crc==crc_) {
		isValid[slot]=true;
		return PAL_FIRMWARE_SUCCESS;
	} else {
		return PAL_FIRMWARE_CRC_ERROR;
	}
}

palFirmware_ret_t palFirmware_eraseAsync(uint8_t slot, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_writeAsync(const uint8_t *data, palFirmware_slotNum_t slot, palFirmware_segNum_t segIndex, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_readAsync(uint8_t *data, palFirmware_slotNum_t slot, palFirmware_segNum_t segIndex, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_validateAsync(palFirmware_slotNum_t slot, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_run(uint8_t slot) {
	if (slot >=P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}
	if (!isValid[slot]) {
		return PAL_FIRMWARE_INVALID_FIRMWARE;
	}
	palExec_reset();
	cout<<"Load Firmware"<<endl;
	return PAL_FIRMWARE_SUCCESS;

}

cometos_error_t palExec_reset() {
	cout<<"reset node"<<endl;
	return COMETOS_SUCCESS;
}

palFirmware_ret_t palFirmware_initTransferAsync(palFirmware_slotNum_t slot, palFirmware_segNum_t numSegs, palFirmware_callback_t callback, palFirmware_crc_t crc) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}
