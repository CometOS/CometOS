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
 Bootloader of Fail-Fafe OTAP. Read the specification
 failsafe bootloader for further information.


 \li activate boot reset (BOOTRST fuse)
 \li activate watchdog via fuses
 \li set bootloader size to 8KB (BOOTSZx fuses)
 \li  use "-Wl,--section-start=.text=0x1E000" as linker argument for GCC (is done in Makefile)
 \li bootloader has to be flashed via ISP

 */

/*INCLUDES-------------------------------------------------------------------*/

#include "bootloader.h"
#include "bootloaderMappings.h"
#include "palPers.h"
#include "resetInformation.h"
#include "palFirmware.h"

#include <util/delay.h>

/*MACROS---------------------------------------------------------------------*/

#define WATCHDOG_PERIOD WDTO_4S

// in milliseconds
#define ENERGY_CHECK_TIME	2000

#if SPM_PAGESIZE != P_FIRMWARE_SEGMENT_SIZE
    #error "SPM_PAGESIZE has to be equal to P_FIRMWARE_SEGMENT_SIZE"
#endif

/*FUNCTION DEFINITION--------------------------------------------------------*/

/**
 * Currently an energy check is performed when starting the MCU. It is needed
 * to protect the EEPROM for repeated writing access which can not be finished
 * due to low energy conditions (e.g. solar harvester).
 *
 * Since the MCU can not access the power level, this function runs the MCU on
 * a very high power level in order to detect energy drop-off.
 *
 * currently not
 */
//static void energyCheck() {
//	palLed_on(0xff);
//	for (uint16_t i = 0; i < ENERGY_CHECK_TIME; i++) {
//		_delay_ms(1);
//		wdt_reset();
//	}
////	palLed_off(0);
//}

void boot_getResetInfo(void) __attribute__((naked)) __attribute__((section(".init1")));

void boot_getResetInfo(void) {
    // here we store some data at some point in memory (e.g. the begin
    // of the .data section), for the firmware to readout
    COMETOS_BOOT_MCUSR = MCUSR;
    COMETOS_BOOT_GET_PC(COMETOS_BOOT_PC);
    COMETOS_BOOT_CHKSUM = COMETOS_BOOT_CALC_CHKSUM;

    // disable watchdog, NOTE that setting MCUSR is necessary, because
    // wdt_disable() does not reset the WDRF flag; but this flag
    // overrides WDE of WDTCSR and thereby always enables the watchdog if
    // it is set
    MCUSR = 0;
    wdt_disable();
}

/**
 */
int main() {
	wdt_enable(WATCHDOG_PERIOD);
	wdt_reset();

	// initialize everything
	palLed_init();
	palLed_off(0xFF);
	INIT_FIRMWARE_STORAGE();
//wdt_reset();
// energy drop-off detection
//	energyCheck();
	// check whether to load new firmware
	uint8_t opcode = eeprom_read_byte((uint8_t*) P_FIRMWARE_OPCODE);
	if ((0xF & opcode) ==  (((~opcode) >> 4)&0xF)
			&& palPers_getBit(P_FIRMWARE_VALID, 0xF & opcode)) {
		palLed_on(2);
	
		boot_copyFirmwareToFlash(0, (0xF & opcode) * P_FIRMWARE_NUM_SEGS_IN_STORAGE,
				P_FIRMWARE_NUM_SEGS_IN_ROM);
		
		eeprom_write_byte((uint8_t*) P_FIRMWARE_OPCODE, 0xFF);
	} else {
		palLed_on(1);
	}
	for (uint8_t i = 0; i < 4; i++) {
		wdt_reset();
		palLed_toggle(4);
		_delay_ms(1000);
	}	
	boot_start();
	return 0;
}

ISR( WDT_vect, ISR_NAKED) {
	while(true) {
		_delay_ms(2000);
		palLed_toggle(2);
	}
}


