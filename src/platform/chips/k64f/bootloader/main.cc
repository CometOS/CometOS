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

#include "palLed.h"
#include "palPers.h"
#include "bootloader.h"
#include "OutputStream.h"
#include "palFirmware.h"

using namespace cometos;

int main() {
	palLed_init();
	palLed_off(0xFF);

	// check whether to load new firmware
    uint8_t opcode;
    palPers_read(P_FIRMWARE_OPCODE, &opcode, 1); 
	if ((0xF & opcode) ==  (((~opcode) >> 4)&0xF)
			&& palPers_getBit(P_FIRMWARE_VALID, 0xF & opcode)) {
		palLed_on(2);
	
		boot_copyFirmwareToFlash(0, (0xF & opcode) * P_FIRMWARE_NUM_SEGS_IN_STORAGE,
				P_FIRMWARE_NUM_SEGS_IN_ROM/(PAL_FIRMWARE_SECTOR_SIZE/P_FIRMWARE_SEGMENT_SIZE));
		
		uint8_t c = 0xFF;
		palPers_write(P_FIRMWARE_OPCODE, &c, 1);
	} else {
		palLed_on(1);
	}

	for (uint32_t i = 0; i < 10; i++) {
		palLed_toggle(1);
	    for (uint64_t j = 0; j < (uint64_t)1000000; j++) {
		    palLed_toggle(4);
        }
	}

	boot_start();

	return 0;
}

