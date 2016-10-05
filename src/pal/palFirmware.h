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
 *
 *
 *
 * A node contains fixed sized slots for storing firmware images. Each firmware image
 * is stored in exactly one slot and has a state field.
 * Depending on the architecture, a bootloader may directly execute a firmware from a slot
 * or copies the code to a predefined position in the program memory. This abstraction
 * layer defines necessary functions to erase, store, administrate and run a firmware.
 *
 *
 * A firmware can be in one of these states:
 * <li>Invalid	(slot contains parts of a firmware, but was not verified)
 * <li>Valid	(slot contains a runnable image)
 *
 * The state of a firmware is changed to Invalid when calling palFirmware_write() or
 * palFirmware_erase(). Only a call of palFirmware_validate() can set a firmware state to Valid.
 *
 * A slot can have the following states:
 * <li>Non-Final
 * <li>Final
 * A final slot can not be modified. Either a fallbackimage is stored in this slot
 * or the memory is used or storing other data. For the latter the firmware state
 * should be fragment to prohibit accidental load of this.
 *
 * The number of slots and the slot size may vary among implementations. However, each slot
 * must consist of segments with a fixed size of 256 Byte. This value is appropriate for
 * most flash memories (page size).
 *
 * Error codes returned by all functions can be vendor specific.
 *
 * Note that CometOS specifies no concrete bootloader which cooperates with this interface.
 */


#ifndef PALFIRMWARE_H_
#define PALFIRMWARE_H_

#include <stdint.h>
#include <stdbool.h>
#include "Callback.h"
#include "cometosError.h"


/**Size of one firmware segement. */
#define P_FIRMWARE_SEGMENT_SIZE	256

typedef enum {
	PAL_FIRMWARE_SUCCESS=0,
	PAL_FIRMWARE_INVALID_SLOT=1,
	PAL_FIRMWARE_INVALID_SEGMENT=2,
	PAL_FIRMWARE_INVALID_FIRMWARE=3,
	PAL_FIRMWARE_CRC_ERROR=4,
	PAL_FIRMWARE_SIZE_ERROR=5,
	PAL_FIRMWARE_ADDRESS_ERROR=6,
	PAL_FIRMWARE_IS_FINAL=7,
	PAL_FIRMWARE_HARDWARE_ERROR=8,
	PAL_FIRMWARE_ERROR=9,
	PAL_FIRMWARE_NOT_SUPPORTED=10,
	PAL_FIRMWARE_CORRUPTED_IMAGE=11, // error detected in already validated firmware
	PAL_FIRMWARE_INVALID_CALLBACK=12,
	PAL_FIRMWARE_BUSY=13,
	PAL_FIRMWARE_VENDOR_OFFSET=16,
} palFirmware_ret_t;


//typedef void (*palFirmware_callback_t)(palFirmware_ret_t error);
typedef cometos::Callback<void(palFirmware_ret_t error)> palFirmware_callback_t;

typedef uint16_t palFirmware_crc_t;
typedef uint8_t palFirmware_slotNum_t;
typedef uint16_t palFirmware_segNum_t;

//-----------------------------------------------------------------------------

/**
 * CRC calculation function used by PAL firmware.
 * (xmodem, polynom is 0x1021)
 */
static inline uint16_t palFirmware_crc_update(uint16_t crc, uint8_t data) {
	crc = crc ^ ((uint16_t)data << 8);
	for (uint8_t i=0; i<8; i++)
		if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
		else crc <<= 1;
	return crc;
}


//-----------------------------------------------------------------------------


palFirmware_ret_t palFirmware_init();

//-----------------------------------------------------------------------------

/**Firmware state
 * @return firmware state, in case of an invalid slot index false is returned
 */
bool palFirmware_isValid(uint8_t slot);

/**Slot state
 * @return slot state, in case of an invalid slot index true is returned
 */
bool palFirmware_isFinal(uint8_t slot);



//-----------------------------------------------------------------------------

/**
 * @return number of valid slots
 */
uint8_t palFirmware_getNumSlots();

/**@return slot size in number of segments*/
uint16_t palFirmware_getSlotSize();


//-----------------------------------------------------------------------------

/**
 * Erases the given segment of a firmware. Value of memory is
 * assummed to be 0xff
 *
 * @param slot 			selected slot
 * @return 0 in case of no error
 * */
palFirmware_ret_t palFirmware_erase(uint8_t slot);




/***/
palFirmware_ret_t palFirmware_write(const uint8_t *data,uint8_t slot, uint16_t segIndex);


/***/
palFirmware_ret_t palFirmware_read(uint8_t *data, uint8_t slot, uint16_t segIndex);


//-----------------------------------------------------------------------------

/**
 * Depending on the architecture a firmware is executed directly in the memory
 * where it is stored or the bootloader copies the code to some other destination.
 * Since currently no dynamic linking is supported, a linked firmware image can
 * not be stored in each slot. For example, if internally slot 1 starts at
 * address 0x1000 and the code is executed at this position, the selected begin
 * of the linked firmware should also be 0x1000. For this reason the
 * start_address of the firmware image (e.g. first address of the intel-hex) file
 * must be passed to this function. Thus the system can check whether the firmware
 * can be correctly loaded.
 *
 * Note that CRC is compared with calculated CRC of whole slot. Segments that were
 * not modified can be assumed to have constant 0xFF value.
 *
 * It is recommend to store the passed CRC checksum. This can later be used to
 * validate a firmware before running it in order to cope with corrupted firmware
 * images (e.g. accidental modification of persistent memory by user).
 *
 * @param slot
 * @param start_address	start_address selected by linker
 * @param crc, 16 bit CRC (xmodem, polynom is 0x1021, initial crc must be 0xffff)
 * @return 0 in case of no error
 */
palFirmware_ret_t palFirmware_validate(uint8_t slot, uint32_t address_offset, uint16_t crc);


// NEW ASYNCHRONOUS INTERFACE, CURRENTLY TESTING THIS
palFirmware_ret_t palFirmware_initTransferAsync(palFirmware_slotNum_t slot, palFirmware_segNum_t numSegs, palFirmware_callback_t callback, palFirmware_crc_t crc);

palFirmware_ret_t palFirmware_writeAsync(const uint8_t *data, palFirmware_slotNum_t slot, palFirmware_segNum_t segIndex, palFirmware_callback_t callback);

palFirmware_ret_t palFirmware_readAsync(uint8_t *data, palFirmware_slotNum_t slot, palFirmware_segNum_t segIndex, palFirmware_callback_t callback);

//palFirmware_ret_t palFirmware_validateAsync(palFirmware_slotNum_t slot, uint32_t address_offset, palFirmware_crc_t crc, palFirmware_callback_t callback);
palFirmware_ret_t palFirmware_validateAsync(palFirmware_slotNum_t slot, palFirmware_callback_t callback);


//-----------------------------------------------------------------------------


/**
 * In case of no error, this function always resets the node
 * and runs the specified firmware
 *
 *  @return 0 in case of no error
 */
palFirmware_ret_t palFirmware_run(uint8_t slot);

//-----------------------------------------------------------------------------

/**
 * Only for internal use.
 */
palFirmware_ret_t palFirmwareImpl_init();

/**
 * Only for internal use.
 */
uint8_t palFirmwareImpl_getNumSectors();

/**
 * Only for internal use.
 */
cometos_error_t palFirmwareImpl_sectorEraseAsync(uint32_t addr, cometos::Callback<void(cometos_error_t)> callback);

/**
 * Only for internal use.
 */
cometos_error_t palFirmwareImpl_pageProgramAsync(uint32_t addr, const uint8_t * buf, uint16_t len, cometos::Callback<void(cometos_error_t)> callback);

/**
 * Only for internal use.
 */
uint32_t palFirmwareImpl_getSectorSize();

/**
 * Only for internal use.
 */
cometos_error_t palFirmwareImpl_readAsync(uint32_t addr, uint8_t * buf, uint16_t len, cometos::Callback<void(cometos_error_t)> cb);


#endif /* PALFIRMWARE_H_ */
