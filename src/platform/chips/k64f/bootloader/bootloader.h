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

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "palLed.h"
#include "bootloaderMappings.h"
//#include "twi_eeprom.h"

#include "palFirmwareDefs.h"


/*PROTOTYPES-----------------------------------------------------------------*/


/**
 * Start application. Clears global interrupt flag and
 * jumps to position 0;
 */
void boot_start();

/**
 * Copies data from external firmware storage to program memory. Functions
 * transfers only pages of size of SPM_PAGESIZE.
 *
 * @start_page_pgm		start page in pgm for writing data
 * @startPageFwStorage	start page (in same units as pgm)
 * @numPages			number of pages to copy
 *
 * Note that external storage must be initialized before calling this
 * function.
 */
void boot_copyFirmwareToFlash(uint16_t start_page_pgm,
		uint16_t startPageFwStorage, uint16_t numPages);




/**
 * Reads data from pgm space.
 *
 * @param address	address in program memory
 * @param data		buffer to store read data
 * @param length	number of bytes to read
 */
void boot_read(uint32_t address, uint8_t *data, uint16_t length);


/**
 * Reads data from pgm space and compars it with give string
 *
 * @param address	address in program memory
 * @param data		buffer which should be compared with read data
 * @param length	number of bytes to compare
 * @return true of entire array mathces with eeprom
 */
bool boot_verify(uint32_t address, uint8_t *data, uint16_t length);


/**
 * Writes page to pgm space.
 *
 * @param address	address in program memory (e.g. start of page)
 * @param data		buffer from which data is read, length is assumed to
 * 					be SPM_PAGESIZE!
 */
void boot_write(uint32_t address, const uint8_t *data);

#endif /* BOOTLOADER_H_ */
