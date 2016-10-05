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

/*INCLUDES-------------------------------------------------------------------*/

#include <string.h>
#include "bootloader.h"
#include "OutputStream.h"

extern "C" {
#include "fnet.h"
#include "fnet_flash.h"
}

using namespace cometos;

/*FUNCTION DEFINITION--------------------------------------------------------*/

extern char Reset_Handler;

void boot_start() {
	uintptr_t address = (uintptr_t)(&Reset_Handler);
	address += FIRMWARE_OFFSET;
    address = ((address)|0x1); // set the thumb bit
    (( void(*)() )address)();  // Jump
}

void boot_copyFirmwareToFlash(uint16_t start_page_pgm,
		uint16_t startPageFwStorage, uint16_t numPages) {

	uint8_t data[PAL_FIRMWARE_SECTOR_SIZE];

	uint32_t addrFwStorage = PAL_FIRMWARE_SECTOR_SIZE;
	addrFwStorage *= startPageFwStorage;
	uint32_t addrPgm = PAL_FIRMWARE_SECTOR_SIZE;
	addrPgm *= start_page_pgm;

	while (numPages > 0) {
		// read data
		palLed_on(6);
        uint8_t* mem = ((uint8_t*)PAL_FIRMWARE_ADDRESS)+addrFwStorage;
        memcpy(data, mem, PAL_FIRMWARE_SECTOR_SIZE);
		palLed_off(6);

		// then write data
		boot_write(addrPgm, data);
		addrFwStorage += PAL_FIRMWARE_SECTOR_SIZE;
		addrPgm += PAL_FIRMWARE_SECTOR_SIZE;

		//wdt_reset();
		palLed_toggle(1);

		numPages--;
	}
}

#if 0
void boot_read(uint32_t address, uint8_t *data, uint16_t length) {
	uint16_t i;
	for (i = 0; i < length; i++) {
		data[i] = pgm_read_byte_far(address);
		address++;
	}
}

bool boot_verify(uint32_t address, uint8_t *data, uint16_t length) {
	uint16_t i;
	for (i = 0; i < length; i++) {
		if (data[i] != pgm_read_byte_far(address)) {
			return false;
		}
		address++;
	}
	return true;
}
#endif

void boot_write(uint32_t address, const uint8_t *data) {
    uint8_t* mem = ((uint8_t*)FIRMWARE_OFFSET)+address;
    fnet_flash_erase( (void *)(mem), PAL_FIRMWARE_SECTOR_SIZE);
    fnet_flash_memcpy( (void *)(mem), data, PAL_FIRMWARE_SECTOR_SIZE);
	palLed_off(2);
}

