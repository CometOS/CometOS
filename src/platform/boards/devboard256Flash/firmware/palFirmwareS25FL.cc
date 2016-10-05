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
 * @author Stefan Unterschuetz
 */

#include "palExec.h"
#include "palFirmware.h"
#include "palFirmwareDefs.h"
#include "palPers.h"
#include "palLed.h"
#include "s25fl_blocking.h"
#include <string.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "timer1.h"

#if ((P_FIRMWARE_SEGMENT_SIZE * P_FIRMWARE_NUM_SEGS_IN_STORAGE) % HAL_S25FL_SECTOR_SIZE) != 0
#error "Firmware size has to be multiple of S25FL flash memory sector size, i.e., of 65536 bytes"
#endif

#if (P_FIRMWARE_SEGMENT_SIZE != HAL_S25FL_PAGE_SIZE)
#error "Segment size has to agree with page size of flash memory (256 bytes)"
#endif

#if (P_FIRMWARE_SEGMENT_SIZE * P_FIRMWARE_NUM_SEGS_IN_STORAGE * P_FIRMWARE_NUM_SLOTS > HAL_S25FL_SIZE)
#error "Memory reserved for firmware is larger than target flash memory"
#endif

const uint8_t FIRMWARE_S25FL_SECTORS = (((uint32_t)P_FIRMWARE_SEGMENT_SIZE) * P_FIRMWARE_NUM_SEGS_IN_STORAGE) / HAL_S25FL_SECTOR_SIZE;

// TODO this contains much redundancy with the externaleeprom-based
//      palFirmware implementation of the devboard (except erase) and
//      should be moved into a better abstraction to avoid code redundancy;

static uint8_t upperSeg;


palFirmware_ret_t palFirmware_init() {
	if (COMETOS_SUCCESS != s25fl_init()) {
	    return PAL_FIRMWARE_ERROR;
	}
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

	uint32_t currAddr;
    currAddr = P_FIRMWARE_SEGMENT_SIZE;
    currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;

    palLed_on(1);
	for (uint8_t i = 0; i < FIRMWARE_S25FL_SECTORS; i++) {
	    palLed_toggle(2);
	    wdt_reset();
	    if (COMETOS_SUCCESS != s25fl_sectorErase(currAddr)) {
	        return PAL_FIRMWARE_HARDWARE_ERROR;
	    }
	    wdt_reset();
	    currAddr += HAL_S25FL_SECTOR_SIZE;
	}
	palLed_off(2);

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
	
	if (COMETOS_SUCCESS != s25fl_pageProgram(address, data, P_FIRMWARE_SEGMENT_SIZE)) {
	    return PAL_FIRMWARE_HARDWARE_ERROR;
	} else {
	    return PAL_FIRMWARE_SUCCESS;
	}
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
		if (COMETOS_SUCCESS != s25fl_read(address, data, P_FIRMWARE_SEGMENT_SIZE)) {
		    return PAL_FIRMWARE_HARDWARE_ERROR;
		}

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


	eeprom_write_byte((uint8_t*) P_FIRMWARE_OPCODE, (slot&0x0f) | ((~(slot<<4))&0xf0));


	palExec_reset();

	return PAL_FIRMWARE_ERROR;
}


