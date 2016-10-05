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


/*FUNCTION DEFINITION--------------------------------------------------------*/

void boot_start() {
	__asm__ __volatile__ ("jmp 0" ::);
}

void boot_copyFirmwareToFlash(uint16_t start_page_pgm,
		uint16_t startPageFwStorage, uint16_t numPages) {

	uint8_t data[SPM_PAGESIZE];

	uint32_t addrFwStorage = SPM_PAGESIZE;
	addrFwStorage *= startPageFwStorage;
	uint32_t addrPgm = SPM_PAGESIZE;
	addrPgm *= start_page_pgm;

	while (numPages > 0) {
		// read data from external eeprom
		palLed_on(6);
		READ_FROM_FIRMWARE_STORAGE(addrFwStorage, data, SPM_PAGESIZE);
		palLed_off(6);
		// then write data
		boot_write(addrPgm, data);
		addrFwStorage += SPM_PAGESIZE;
		addrPgm += SPM_PAGESIZE;

		wdt_reset();
		palLed_toggle(1);

		numPages--;
	}
}

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

void boot_write(uint32_t address, const uint8_t *data) {

	uint16_t i;
	uint16_t word;
	// busy wait important, flash must not be written while 
	// eeprom action is still active
	eeprom_busy_wait();
	boot_page_erase(address);
	boot_spm_busy_wait();

	// fill cache of program memory
	for (i = 0; i < SPM_PAGESIZE; i += 2) {
		word = *data;
		data++;
		word += (*data) << 8;
		data++;
		boot_page_fill(address + i, word);
	}

	// write page
	boot_page_write(address);
	boot_spm_busy_wait();
	boot_rww_enable();
	palLed_off(2);

}

