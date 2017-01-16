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
 * CometOS Platform Abstraction Layer for Firmware Updates.
 *
 * @author Stefan Unterschuetz, Andreas Weigel
 */

#include "palFirmware.h"
#include "palFirmwareDefs.h"
#include "palLed.h"
#include "palPers.h"
#include "twi_eeprom.h"
#include <string.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

static uint8_t upperSeg;

palFirmware_ret_t palFirmware_init() {
	ext_eeprom_init();
	upperSeg = 0;
	return PAL_FIRMWARE_SUCCESS;
}

bool palFirmware_isValid(uint8_t slot) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return false;
	}
	return palPers_getBit(P_FIRMWARE_VALID, slot);
}

bool palFirmware_isFinal(uint8_t slot) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return true;
	}
	return false;
}

uint8_t palFirmware_getNumSlots() {
	return P_FIRMWARE_NUM_SLOTS;
}

uint16_t palFirmware_getSlotSize() {
	return P_FIRMWARE_NUM_SEGS_IN_STORAGE;
}

palFirmware_ret_t palFirmware_erase(uint8_t slot) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (palFirmware_isValid(slot)) {
		palPers_setBit(false, P_FIRMWARE_VALID, slot);
	}

	uint8_t data[P_FIRMWARE_SEGMENT_SIZE];
	memset(data, 0xFF, P_FIRMWARE_SEGMENT_SIZE);
	for (uint16_t i = 0; i < P_FIRMWARE_NUM_SEGS_IN_STORAGE; i++) {
		if (PAL_FIRMWARE_SUCCESS != palFirmware_write(data, slot, i)) {
			palLed_off(0x4);
			return PAL_FIRMWARE_ERROR;
		}
		palLed_toggle(0x4);
		wdt_reset();
	}
	palLed_off(0x4);
	return PAL_FIRMWARE_SUCCESS;
}

palFirmware_ret_t palFirmware_write(const uint8_t *data, uint8_t slot,
		uint16_t segIndex) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (segIndex >= P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
		return PAL_FIRMWARE_INVALID_SEGMENT;
	}

	if (palFirmware_isValid(slot)) {
		palPers_setBit(false, P_FIRMWARE_VALID, slot);
	}

	uint32_t address = P_FIRMWARE_SEGMENT_SIZE;
	address *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;
	address += ((uint32_t) P_FIRMWARE_SEGMENT_SIZE) * segIndex;
	
	ext_eeprom_write(address, data, P_FIRMWARE_SEGMENT_SIZE);

	return PAL_FIRMWARE_SUCCESS;

}

palFirmware_ret_t palFirmware_read(uint8_t *data, uint8_t slot,
		uint16_t segIndex) {
	return PAL_FIRMWARE_NOT_SUPPORTED;

}

palFirmware_ret_t palFirmware_validate(uint8_t slot, uint32_t address_offset,
		uint16_t crc) {

	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	uint32_t address = P_FIRMWARE_SEGMENT_SIZE;
	address *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;

	uint16_t crc_ = 0xFFFF;
	//f("crc_", slot, address);
	uint8_t data[P_FIRMWARE_SEGMENT_SIZE];
	for (uint16_t i = 0; i < P_FIRMWARE_NUM_SEGS_IN_STORAGE; i++) {
		ext_eeprom_read(address, data, P_FIRMWARE_SEGMENT_SIZE);

		for (uint16_t j = 0; j < P_FIRMWARE_SEGMENT_SIZE; j++) {
			crc_ = palFirmware_crc_update(crc_, data[j]);
		}
		//f("crc_", crc_, i);
		palLed_toggle(0x4);
		wdt_reset();
		address += P_FIRMWARE_SEGMENT_SIZE;
	}
	palLed_off(0x4);
	//f("crc_",crc_, 1);
	//f("crc_",crc, 0);
	if (crc_ != crc) {
		return PAL_FIRMWARE_CRC_ERROR;
	}

	palPers_setBit(true, P_FIRMWARE_VALID, slot);

	return PAL_FIRMWARE_SUCCESS;
}

palFirmware_ret_t palFirmware_run(uint8_t slot) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (!palFirmware_isValid(slot)) {
		return PAL_FIRMWARE_INVALID_FIRMWARE;
	}

	uint8_t slotFlag = (slot & 0x0f) | ((~(slot << 4)) & 0xf0);
	        palPers_write(P_FIRMWARE_OPCODE, &slotFlag, 1);
//	eeprom_write_byte((uint8_t*) P_FIRMWARE_OPCODE, (slot&0x0f) | ((~(slot<<4))&0xf0));


	palExec_reset();

	return PAL_FIRMWARE_ERROR;
}


