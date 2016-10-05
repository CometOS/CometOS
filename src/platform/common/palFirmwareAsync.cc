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
 * @author Andreas Weigel
 */

/**
 * @file palFirmwareAsync
 *
 * Abstraction for the firmware update mechanism. Interface is based on slots
 * and segments. Typical flow for an image to be stored:
 * <ul>
 * <li> Initiation, which causes the storage to be prepared (erased), the
 *      given crc to be written to the start of the segment after the last
 *      data segment, also, the size of the image is written to the last two
 *      bytes of the last segment of the slot. Those positions are expected to
 *      be always free, because the size of a slot corresponds to the size of
 *      the ROM on the MCU, which will have at least one segment reserved for
 *      storing some bootloader. The crc is expected to be calculated over the
 *      all the bytes in all segments, starting with 0, padding should be
 *      used if the actual firmware image is not aligned to a segment
 *      boundary.
 * <li> Writing the firmware one segment after the other, each segment being of
 *      size P_FIRMWARE_SEGMENT_SIZE bytes.
 * <li> Verify. While this can and should be done within the bootloader itself,
 *      a verification is expected to be done by this layer after successful
 *      transport. The written data is validated against the CRC and size
 *      transmitted with the initiation of the transfer.
 */

#include "palFirmware.h"
#include "palFirmwareDefs.h"
#include "palPers.h"
#include "palLed.h"
#include "logging.h"
#include "OutputStream.h"
#include <limits.h>
#include <string.h>
//#include <avr/interrupt.h>
//#include <util/delay.h>
#include "cometosAssert.h"
#include "palExecUtils.h"

// TODO this contains much redundancy with the externaleeprom-based
//      palFirmwareAsync implementation of the devboard (except erase) and
//      should be moved into a better abstraction to avoid code redundancy;

static uint8_t upperSeg;

// buffer size which is processed in interrupt
// this is crucial if calculating CRC
#define TEMP_BUFFER_SIZE 16

using namespace cometos;

static palFirmware_callback_t currCallback = EMPTY_CALLBACK();
static uint8_t currData[TEMP_BUFFER_SIZE];
static palFirmware_slotNum_t currSlot;
static palFirmware_segNum_t currNumSegs;
static uint32_t currAddr;
static uint32_t currLength;

enum {
    PF_INIT_ERASE,
    PF_INIT_WRITE_CRC,
    PF_INIT_WRITE_LEN,
    PF_VALI_READ_LEN,
    PF_VALI_READ_IMG
};

struct {
    uint8_t length;
    uint8_t type;
} currJob;

static palFirmware_crc_t currCrc;

palFirmware_ret_t palFirmware_init() {
    palFirmware_ret_t result = palFirmwareImpl_init();
    if (result != PAL_FIRMWARE_SUCCESS) {
        return result;
    }
	upperSeg = 0;
	currCallback = EMPTY_CALLBACK();
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
	currCallback = EMPTY_CALLBACK();
	callb_(error);
}

template<typename T>
static void writeToArrayBe(uint8_t a[], T value) {
    uint8_t idx = sizeof(T) - 1;
    for (uint8_t k = 0; k <= idx; k++) {
        a[idx - k] = value & 0xFF;
        value >>=CHAR_BIT;
    }
}

template<typename T>
static T readFromArrayBe(uint8_t a[]) {
    T value = 0;
    for (uint8_t k = 0; k < sizeof(T); k++) {
        value <<= CHAR_BIT;
        value += a[k];
    }
    return value;
}

static uint32_t getImageSizeAddress(palFirmware_slotNum_t slot) {
    uint32_t lenAddr = P_FIRMWARE_SEGMENT_SIZE;
    lenAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * (slot + 1);
    ASSERT(lenAddr > 0);
    lenAddr -= sizeof(palFirmware_segNum_t);
    return lenAddr;
}

static void initDone(cometos_error_t status) {
	if (status != COMETOS_SUCCESS) {
	    // immediately abort if flash reading was unsuccessful
		callCallback(PAL_FIRMWARE_HARDWARE_ERROR);
	} else {
	    if (currJob.type == PF_INIT_ERASE) {
	        // currently occupied erasing the slot
            currLength++;
            // check if all sectors belonging to this firmware were erased
            if (currLength == palFirmwareImpl_getNumSectors()) {
                // we have successfully erased the slot, now write CRC to kth
                // segment, which is the one after the last image-containing one
                currJob.type = PF_INIT_WRITE_CRC;

                // initialize crcAddr to startaddr of slot plus
                // number of segments * segment size to get to the first free
                // segment of this slot, where we are going to store the crc
                uint32_t crcAddr = P_FIRMWARE_SEGMENT_SIZE;
                crcAddr *=  (P_FIRMWARE_NUM_SEGS_IN_STORAGE * currSlot) + currNumSegs;

                // put CRC at beginning of new segment in big endian
                writeToArrayBe<palFirmware_crc_t>(currData, currCrc);
                LOG_DEBUG(crcAddr << "|" << currNumSegs << "|" << (int) currSlot << "|" << (int) currData[0] << " " << (int) currData[1]);
                if (COMETOS_SUCCESS != palFirmwareImpl_pageProgramAsync(crcAddr, currData, sizeof(currCrc), CALLBACK_FUN(initDone))) {
                    callCallback(PAL_FIRMWARE_HARDWARE_ERROR);
                }
            } else {
                // we need to erase more sectors before slot is clean
                currAddr += palFirmwareImpl_getSectorSize();
                if (COMETOS_SUCCESS != palFirmwareImpl_sectorEraseAsync(currAddr, CALLBACK_FUN(initDone))) {
                    callCallback(PAL_FIRMWARE_HARDWARE_ERROR);
                }
            }
	    } else if (currJob.type == PF_INIT_WRITE_CRC) {
	        // we've written the CRC successfully to the last segment; now we
	        // write the image size in segments to the last bytes of the slot
	        currJob.type = PF_INIT_WRITE_LEN;
	        uint32_t lenAddr = getImageSizeAddress(currSlot);
	        writeToArrayBe<palFirmware_segNum_t>(currData, currNumSegs);
	        LOG_DEBUG(lenAddr << "|" << currNumSegs << "|" << (int) currSlot << "|" << (int) currData[0] << " " << (int) currData[1]);
	        if (COMETOS_SUCCESS != palFirmwareImpl_pageProgramAsync(lenAddr, currData, sizeof(currNumSegs), CALLBACK_FUN(initDone))) {
	            callCallback(PAL_FIRMWARE_HARDWARE_ERROR);
	        }
	    } else if (currJob.type == PF_INIT_WRITE_LEN){
	        // finished erasing slot and writing initial data, raise done event
	        callCallback(PAL_FIRMWARE_SUCCESS);
	    } else {
	        ASSERT(false);
	    }
	}
}

palFirmware_ret_t palFirmware_initTransferAsync(palFirmware_slotNum_t slot,
                                                palFirmware_segNum_t numSegs,
                                                palFirmware_callback_t callback,
                                                palFirmware_crc_t crc) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (!callback) {
		return PAL_FIRMWARE_INVALID_CALLBACK;
	}

	if (currCallback) {
		return PAL_FIRMWARE_BUSY;
	}

	if (numSegs < 1 || numSegs > P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
	    return PAL_FIRMWARE_SIZE_ERROR;
	}

	if (palFirmware_isValid(slot)) {
		palPers_setBit(false, P_FIRMWARE_VALID, slot);
	}

	currCallback = callback;

	currJob.type = PF_INIT_ERASE;
	currNumSegs = numSegs;
	currSlot = slot;
	currAddr = P_FIRMWARE_SEGMENT_SIZE;
	currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * currSlot;
	currCrc = crc;

	// use currLength parameter to store number of erases
	currLength = 0;

	if (COMETOS_SUCCESS != palFirmwareImpl_sectorEraseAsync(currAddr, CALLBACK_FUN(initDone))) {
	    currCallback = EMPTY_CALLBACK();
	    return PAL_FIRMWARE_HARDWARE_ERROR;
	} else {
	    return PAL_FIRMWARE_SUCCESS;
	}
}

static void writeDone(cometos_error_t status) { //, s25fl_addr_t addr, const uint8_t * buf, s25fl_size_t len) {
	if (status != COMETOS_SUCCESS) {
	    callCallback(PAL_FIRMWARE_ERROR);
	} else {
	    callCallback(PAL_FIRMWARE_SUCCESS);
	}
}

palFirmware_ret_t palFirmware_writeAsync(const uint8_t *data,
                                               palFirmware_slotNum_t slot,
                                               palFirmware_segNum_t segIndex,
                                               palFirmware_callback_t callback) {
	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (segIndex >= P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
		return PAL_FIRMWARE_INVALID_SEGMENT;
	}

	if (!callback) {
		return PAL_FIRMWARE_INVALID_CALLBACK;
	}

	if (currCallback) {
		return PAL_FIRMWARE_BUSY;
	}

	if (palFirmware_isValid(slot)) {
		palPers_setBit(false, P_FIRMWARE_VALID, slot);
	}
	currCallback = callback;
	currAddr = P_FIRMWARE_SEGMENT_SIZE;
	currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * slot;
	currAddr += ((uint32_t) P_FIRMWARE_SEGMENT_SIZE) * segIndex;

	if (COMETOS_SUCCESS != palFirmwareImpl_pageProgramAsync(currAddr, data, P_FIRMWARE_SEGMENT_SIZE, CALLBACK_FUN(writeDone))) {
	    // clear currCallback, which is used as busy indicator!
	    currCallback = EMPTY_CALLBACK();
	    return PAL_FIRMWARE_HARDWARE_ERROR;
	} else {
	    return PAL_FIRMWARE_SUCCESS;
	}
}

palFirmware_ret_t palFirmware_readAsync(uint8_t *data, uint8_t slot,
		uint16_t segIndex, palFirmware_callback_t callback) {
	return PAL_FIRMWARE_NOT_SUPPORTED;

}

static void validateDone(cometos_error_t status) {
	if (status != COMETOS_SUCCESS) {
		callCallback(PAL_FIRMWARE_ERROR);
		return;
	} else {
	    if (currJob.type == PF_VALI_READ_LEN) {
	        currAddr = P_FIRMWARE_SEGMENT_SIZE;
            currAddr *= P_FIRMWARE_NUM_SEGS_IN_STORAGE * currSlot;

            // now get size of image from read data
            currNumSegs = readFromArrayBe<palFirmware_segNum_t>(currData);

            // sanity check on the length read
            if (currNumSegs > P_FIRMWARE_NUM_SEGS_IN_STORAGE) {
                callCallback(PAL_FIRMWARE_INVALID_FIRMWARE);
                return;
            }

            currLength = P_FIRMWARE_SEGMENT_SIZE;
            currLength *= currNumSegs;
            // add additional CRC bytes
            currLength += sizeof(palFirmware_crc_t);
            currJob.length = 0;
            currJob.type = PF_VALI_READ_IMG;
            LOG_DEBUG(currAddr << "|" << currLength << "|" << currNumSegs << "|" << (int) currData[0] << "|" << (int) currData[1]);
            // we just call us again to actually start working
            validateDone(COMETOS_SUCCESS);
	    } else if (currJob.type == PF_VALI_READ_IMG) {
	        for (uint8_t i = 0; i < currJob.length; i++) {
                currCrc = palFirmware_crc_update(currCrc, currData[i]);
            }
	        ASSERT(currLength >= currJob.length);
            currLength -= currJob.length;
            currAddr += currJob.length;

            if (currLength == 0) {
                if (currCrc == 0) {
                    palPers_setBit(true, P_FIRMWARE_VALID, currSlot);
                    callCallback(PAL_FIRMWARE_SUCCESS);
                } else {
                    callCallback(PAL_FIRMWARE_CRC_ERROR);
                }
                return;
            }

            if (currLength < TEMP_BUFFER_SIZE) {
                currJob.length = currLength;
            } else {
                currJob.length = TEMP_BUFFER_SIZE;
            }

            palFirmwareImpl_readAsync(currAddr, currData, currJob.length, CALLBACK_FUN(validateDone));
	    }
	}
}

palFirmware_ret_t palFirmware_validateAsync(palFirmware_slotNum_t slot, palFirmware_callback_t callback) {

	if (slot >= P_FIRMWARE_NUM_SLOTS) {
		return PAL_FIRMWARE_INVALID_SLOT;
	}

	if (!callback) {
		return PAL_FIRMWARE_INVALID_CALLBACK;
	}

	if (currCallback) {
		return PAL_FIRMWARE_BUSY;
	}
    
    currCallback = callback;
    currCrc = 0xffff;
    currSlot = slot;

	// first we have to get the actual length of the firmware in the slot
    currAddr = getImageSizeAddress(currSlot);
    currJob.type = PF_VALI_READ_LEN;
    // we assume TEMP_BUFFER_SIZE to be large enough to hold the number of segs
    ASSERT(sizeof(currData) >= sizeof(palFirmware_segNum_t));

    LOG_DEBUG(currAddr << "|" << sizeof(palFirmware_segNum_t));
    if (COMETOS_SUCCESS != palFirmwareImpl_readAsync(currAddr, currData, sizeof(palFirmware_segNum_t), CALLBACK_FUN(validateDone))) {
        currCallback = EMPTY_CALLBACK();
        return PAL_FIRMWARE_HARDWARE_ERROR;
    } else {
        return PAL_FIRMWARE_SUCCESS;
    }
}

static void runValidateDone(palFirmware_ret_t status) {
    if (status == PAL_FIRMWARE_SUCCESS) {
        uint8_t slotFlag = (currSlot & 0x0f) | ((~(currSlot << 4)) & 0xf0);
        palPers_write(P_FIRMWARE_OPCODE, &slotFlag, 1);
		palExec_reset();
    } else {
        palLed_toggle(4);
        // TODO: we do not run the faulty firmware, but do not yet cause any
        //       error event to be triggered
    }
}

palFirmware_ret_t palFirmware_run(palFirmware_slotNum_t slot) {
	if (!palFirmware_isValid(slot)) {
		return PAL_FIRMWARE_INVALID_FIRMWARE;
	}

	currSlot = slot;
	palFirmware_ret_t result = palFirmware_validateAsync(currSlot, CALLBACK_FUN(runValidateDone));

	return result;

}

