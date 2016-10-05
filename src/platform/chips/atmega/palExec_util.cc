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
 * @author Andreas Weigel
 */

#include "palExecUtils.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include "palPers.h"
#include "resetInformation.h"
#include <util/delay.h>
#include "palLed.h"
#include "OutputStream.h"
#include "cometosError.h"

static uint8_t resetStatus __attribute__ ((section (".noinit")));
static uint32_t pc __attribute__ ((section (".noinit")));

void getResetInfo(void) __attribute__((naked)) __attribute__((section(".init1")));

#ifndef ATMEGA_PC_WIDTH
#error "ATMEGA_PC_WIDTH has to be defined and is expected to be 2 or 3"
#endif

void palExec_badisr() {
    palLed_init();
    uint8_t count = 0;
    while(1) {
        _delay_ms(100);
        palLed_toggle(0xFF);
        if(++count % 100 == 0) {
                //cometos::getSerialCout() << "BADISR ";
        }
    }
}

ISR(BADISR_vect) {
    palExec_badisr();
}

//#define DDR(x) (*(&(x) - 1))
//void get_mcusr(void) {
//    //DDR(PORTG) |= 1 << 1;
//    //PORTG |= 1 << 1;
//    //PORTG &= ~(1 << 1);
//
//    //DDR(PORTG) |= 1 << 2;
//    //PORTG |= 1 << 2;
//    //PORTG &= ~(1 << 2);
//    //DDR(PORTG) |= 1 << 5;
//    //PORTG |= 1 << 5;
//    //PORTG &= ~(1 << 5);
//}



/*
 * ATTENTION! Careful when changing anything here! Coupled with definitions and
 * algorithms in a bootloader, which is not updated together with a firmware
 * update!
 */
void getResetInfo(void) {
    if (COMETOS_BOOT_CHECK == 0) {
        // bootloader saved MCUSR, PC --> read values and invalidate chksum
        resetStatus = COMETOS_BOOT_MCUSR;
        pc = COMETOS_BOOT_PC;
        COMETOS_BOOT_CHKSUM += 1;
    } else {
        resetStatus = MCUSR;
        COMETOS_BOOT_GET_PC(pc);
    }
//    if (MCUSR == 0) {
//        resetStatus = mcu_reg_content;
//    } else {
//        resetStatus = MCUSR;
//    }
    MCUSR = 0;
    wdt_disable();
//    uint8_t * stackPointer= ((uint8_t*)(SP))+1;
//    pc = 0;
//    pc = (((uint32_t) (*(stackPointer) & 0x1)) << 17) | (((uint32_t) *(stackPointer+1)) << 9) | (((uint32_t) *(stackPointer+2)) << 1);
}

cometos_error_t palExec_reset() {
    wdt_enable(WDTO_4S);
    wdt_reset();
    cli();
        while (1) {
            // wait for watchdog reset
        }
    return COMETOS_ERROR_FAIL;
}

uint8_t palExec_getResetReason() {
    return resetStatus;
}

uint32_t palExec_getPcAtReset() {
    uint8_t wdtFlag;
    uint32_t pcVal = pc;
    palPers_read(PM_WATCHDOG_RESET_FLAG, &wdtFlag, 1);
    if (wdtFlag == 1) {
#if ATMEGA_PC_WIDTH == 2
        uint8_t pc[2];
        palPers_read(PM_WATCHDOG_PC, pc, 2);
        pcVal = (((uint16_t)pc[0]) << 9) | (((uint16_t) pc[1]) << 1);
#elif ATMEGA_PC_WIDTH == 3
        uint8_t pc[3];
        palPers_read(PM_WATCHDOG_PC, pc, 3);
        pcVal = (((uint32_t) (pc[0] & 0x1)) << 17) | (((uint32_t) pc[1]) << 9) | (((uint32_t) pc[2]) << 1);
#endif
    }
    return pcVal;
}

bool palExec_writePc(uint8_t * pc) {
#if ATMEGA_PC_WIDTH == 2
    palPers_write(PM_WATCHDOG_PC, pc, 2);
    return true;
#elif ATMEGA_PC_WIDTH == 3
    palPers_write(PM_WATCHDOG_PC, pc, 3);
    return true;
#else
    return false;
#endif
}

void palExec_clearResetStatus() {
    pc = 0;
    resetStatus = 0;
    palPers_write(PM_WATCHDOG_RESET_FLAG, 0, 1);
}


bool palExec_readAssertInfoShort(uint16_t & line, uint16_t & fileId) {
    uint16_t check = eeprom_read_word((uint16_t*) EEPROM_LOG_ASSERT_ADDRESS+2);
    if (check == 0xFFFF) {
        return false;
    }
#ifdef ASSERT_SHORT
    line = eeprom_read_word((uint16_t*) EEPROM_LOG_ASSERT_ADDRESS);
    fileId = check;
    return true;
#else
    return false;
#endif
}

bool palExec_readAssertInfoLong(uint16_t & line, char * filename, uint8_t fileLen) {
#ifndef ASSERT_SHORT
    uint16_t checkField = eeprom_read_word((uint16_t*) EEPROM_LOG_ASSERT_ADDRESS+2);
    if (checkField != 0xFFFF) {
        line = eeprom_read_word((uint16_t*) EEPROM_LOG_ASSERT_ADDRESS);
        uint16_t len = eeprom_read_word((uint16_t*) (EEPROM_LOG_ASSERT_ADDRESS+2));
		int16_t advance = ((int16_t)len)-((int16_t)fileLen)+1;
		advance = advance <= 0 ? 0 : advance;
        uint8_t* p = (uint8_t*) (EEPROM_LOG_ASSERT_ADDRESS + 4) + advance;
        uint8_t val = eeprom_read_byte(p++);
        uint8_t k = 0;
        while (k < fileLen-1 && val != 0 && val != 0xFF) {
            // Removing the first symbols only containing "../../..//"
            if ((k > 0) || (val != '.' && val != '/' && val != '\\')) {
                filename[k] = val;
                k++;
            }
            val = eeprom_read_byte(p++);
        }
		filename[fileLen-1] = '\0';
        return true;
    } else {
        return false;
    }
#else
    return false;
#endif
}

bool palExec_writeAssertInfoShort(uint16_t line, uint16_t fileId) {
    eeprom_write_word((uint16_t*)EEPROM_LOG_ASSERT_ADDRESS, line);
    eeprom_write_word((uint16_t*)EEPROM_LOG_ASSERT_ADDRESS+2, fileId);
    return true;
}

bool palExec_writeAssertInfoLong(uint16_t line, const char * filename) {
    eeprom_write_word((uint16_t*)EEPROM_LOG_ASSERT_ADDRESS,line);
    eeprom_write_word((uint16_t*)(EEPROM_LOG_ASSERT_ADDRESS+2),strlen(filename));
    eeprom_write_block(filename, (uint8_t*)EEPROM_LOG_ASSERT_ADDRESS+4, strlen(filename)+1);
    return true;
}

bool palExec_clearAssertInfo() {
    eeprom_write_word((uint16_t*)EEPROM_LOG_ASSERT_ADDRESS,0x0);
    eeprom_write_word((uint16_t*)EEPROM_LOG_ASSERT_ADDRESS+2,0xFFFF);
    return true;
}
