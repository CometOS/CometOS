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

#include <avr/eeprom.h>
#include "palPers.h"

#define AVR128RFA1_EEPROM_START 0
#define AVR128RFA1_EEPROM_SIZE 0x1000

/**
 * Initializes module.
 */
void palPers_init() {
    // nothing for internal eeprom
}

/**Persistently writes data to non-volatile memory.
 *
 * @param addr      start address
 * @param buffer    pointer to buffer
 * @param length    length of buffer
 */
void palPers_write(uint16_t addr, const uint8_t* buffer, uint8_t length) {
    for (uint8_t i = 0; i < length && addr+i < AVR128RFA1_EEPROM_SIZE; i++) {
        eeprom_write_byte((uint8_t*) (addr+i), buffer[i]);
    }
}


/**Reads data from a non-volatile memory.
 *
 * @param addr      start address
 * @param buffer    pointer to buffer
 * @param length    length of buffer
 */
void palPers_read(uint16_t addr, uint8_t* buffer, uint8_t length) {
    for (uint8_t i = 0; i < length && addr+i < AVR128RFA1_EEPROM_SIZE; i++) {
        buffer[i] = eeprom_read_byte((uint8_t *) (addr + i));
    }
}


bool palPers_getBit(uint16_t address, uint8_t i) {
    return 0x1 & (eeprom_read_byte((uint8_t*) address) >> i);

}

void palPers_setBit(bool value, uint16_t address, uint8_t i) {
    uint8_t byte = eeprom_read_byte((uint8_t*) address);
    if (value) {
        byte |= (1 << i);
    } else {
        byte &= ~(1 << i);
    }
    eeprom_write_byte((uint8_t*) address, byte);
}

