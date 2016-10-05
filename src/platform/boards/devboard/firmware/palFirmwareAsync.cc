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

#include "palFirmware.h"
#include "palFirmwareDefs.h"
#include "palLed.h"
#include "palPers.h"
#include "eepromAsync.h"
#include <string.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

static uint8_t upperSeg;

#error "Needs to be updated to new validation scheme and merged\
        with src/platform/devboard256Flash/firmware/palFirmwareAsyncS25Fl\
        to avoid redundancy; as a workaround, use synchronous palFirmware\
        for devboard until this happens"

#if ((P_FIRMWARE_SEGMENT_SIZE * P_FIRMWARE_NUM_SEGS_IN_STORAGE) % HAL_S25FL_SECTOR_SIZE) != 0
#error "Firmware size has to be multiple of S25FL flash memory sector size, i.e., of 65536 bytes"
#endif

#if (P_FIRMWARE_SEGMENT_SIZE != HAL_S25FL_PAGE_SIZE)
#error "Segment size has to agree with page size of flash memory (256 bytes)"
#endif

#if (P_FIRMWARE_SEGMENT_SIZE * P_FIRMWARE_NUM_SEGS_IN_STORAGE * P_FIRMWARE_NUM_SLOTS > HAL_S25FL_SIZE)
#error "Memory reserved for firmware is larger than target flash memory"
#endif

#ifndef PAL_FIRMWARE_TIMESTAMP
#define PAL_FIRMWARE_TIMESTAMP 0
#endif
#ifndef PAL_FIRMWARE_VERSION
#define PAL_FIRMWARE_VERSION 0
#endif

const uint8_t FIRMWARE_S25FL_SECTORS = (((uint32_t)P_FIRMWARE_SEGMENT_SIZE) * P_FIRMWARE_NUM_SEGS_IN_STORAGE) / HAL_S25FL_SECTOR_SIZE;


#ifndef PAL_FIRMWARE_VERSION
#error "PAL_FIRMWARE_VERSION has to be defined!"
#endif
static const palFirmware_version_t palFirmware_version = PAL_FIRMWARE_VERSION;

palFirmware_version_t palFirmware_getVersion() {
    return palFirmware_version;
}

// buffer size which is processed in interrupt
// this is crucial if calculating CRC
#define TEMP_BUFFER_SIZE 8

static palFirmware_callback_t currCallback = NULL;
static uint8_t currData[TEMP_BUFFER_SIZE];
static uint8_t currSlot;
static uint32_t currAddr;
static uint32_t currLength;
static uint8_t currLastJobLength;
static uint16_t currCrc;
static uint16_t currCrcTarget;

palFirmware_ret_t palFirmware_init() {
	eepromAsync_init();
	upperSeg = 0;
	currCallback = NULL;
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

static void callCallback(palFirmware_ret_t error) {
	palFirmware_callback_t callb_ = currCallback;
	currCallback = 0;
	callb_(error);
}

static void eraseDone(bool success) {
	if (!success) {
		callCallback(PAL_FIRMWARE_ERROR);
		return;
	}

	if (currLength == 0) {
		callCallback(PAL_FIRMWARE_SUCCESS);
		return;
	}

	if (currLength < TEMP_BUFFER_SIZE) {
		eepromAsync_write(currAddr, currData, currLength, eraseDone);
		currAddr += currLength;
		currLength = 0;
	} else {
		eepromAsync_write(currAddr, currData, TEMP_BUFFER_SIZE, eraseDone);
		currAddr += TEMP_BUFFER_SIZE;
		currLength -= TEMP_BUFFER_SIZE;
	}
}

palFirmware_ret_t palFirmware_eraseAsync(uint8_t slot,
		palFirmware_callback_t callback) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (callback == NULL) {
		return PAL_FIRMWARE_INVALID_CALLBACK;
	}

	if (currCallback != NULL) {
		return PAL_FIRMWARE_BUSY;
	}

	if (palFirmware_isValid(slot)) {
		palPers_setBit(false, P_FIRMWARE_VALID, slot);
	}

	memset(currData, 0xFF, TEMP_BUFFER_SIZE);

	currCallback = callback;
	currAddr = P_FIRMWARE_SEGMENT_SIZE;
	currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;
	currLength = P_FIRMWARE_SEGMENT_SIZE;
	currLength *= P_FIRMWARE_NUM_SEGS_IN_STORAGE;
	eraseDone(true);

	return PAL_FIRMWARE_SUCCESS;
}

static void writeDone(bool success) {
	if (success) {
		callCallback(PAL_FIRMWARE_SUCCESS);
	} else {
		callCallback(PAL_FIRMWARE_ERROR);
	}
}

palFirmware_ret_t palFirmware_writeAsync(const uint8_t *data, uint8_t slot,
		uint16_t segIndex, palFirmware_callback_t callback) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (segIndex >= P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
		return PAL_FIRMWARE_INVALID_SEGMENT;
	}

	if (callback == NULL) {
		return PAL_FIRMWARE_INVALID_CALLBACK;
	}

	if (currCallback != NULL) {
		return PAL_FIRMWARE_BUSY;
	}

	if (palFirmware_isValid(slot)) {
		palPers_setBit(false, P_FIRMWARE_VALID, slot);
	}
	currCallback = callback;
	currAddr = P_FIRMWARE_SEGMENT_SIZE;
	currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;
	currAddr += ((uint32_t) P_FIRMWARE_SEGMENT_SIZE) * segIndex;
	eepromAsync_write(currAddr, data, P_FIRMWARE_SEGMENT_SIZE, writeDone);

	return PAL_FIRMWARE_SUCCESS;

}

palFirmware_ret_t palFirmware_readAsync(uint8_t *data, uint8_t slot,
		uint16_t segIndex, palFirmware_callback_t callback) {
	return PAL_FIRMWARE_NOT_SUPPORTED;

}



static void validateDone(bool success) {
	if (!success) {
		callCallback(PAL_FIRMWARE_ERROR);
		return;
	}

	for (uint8_t i = 0; i < currLastJobLength; i++) {
		currCrc = palFirmware_crc_update(currCrc, currData[i]);
	}
	currLength -= currLastJobLength;
	currAddr += currLastJobLength;

	if (currLength == 0) {

		if (currCrc == currCrcTarget) {
			palPers_setBit(true, P_FIRMWARE_VALID, currSlot);
			callCallback(PAL_FIRMWARE_SUCCESS);
		} else {
			callCallback(PAL_FIRMWARE_CRC_ERROR);
		}
		return;
	}

	if (currLength < TEMP_BUFFER_SIZE) {
		currLastJobLength = currLength;
	} else {
		currLastJobLength = TEMP_BUFFER_SIZE;
	}

	eepromAsync_read(currAddr, currData, currLastJobLength, validateDone);

}

palFirmware_ret_t palFirmware_validateAsync(palFirmware_slotNum_t slot,
		uint32_t address_offset, uint16_t crc, palFirmware_callback_t callback) {

	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (callback == NULL) {
		return PAL_FIRMWARE_INVALID_CALLBACK;
	}

	if (currCallback != NULL) {
		return PAL_FIRMWARE_BUSY;
	}

	currCallback = callback;
	currCrc = 0xffff;
	currCrcTarget = crc;
	currSlot = slot;

	currAddr = P_FIRMWARE_SEGMENT_SIZE;
	currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;
	currLength = P_FIRMWARE_SEGMENT_SIZE;
	currLength *= P_FIRMWARE_NUM_SEGS_IN_STORAGE;
	currLastJobLength = 0;

	validateDone(true);

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
//	eeprom_write_byte((uint8_t*) P_FIRMWARE_OPCODE,
//			(slot & 0x0f) | ((~(slot << 4)) & 0xf0));

	palExec_reset();

	return PAL_FIRMWARE_ERROR;
}

palFirmware_ret_t palExec_reset() {
	wdt_enable(WDTO_4S);
	wdt_reset();
	cli();
	while (1) {
		// wait for watchdog reset
		palLed_toggle(4);
		_delay_ms(25);
	}
	return PAL_FIRMWARE_ERROR;
}

