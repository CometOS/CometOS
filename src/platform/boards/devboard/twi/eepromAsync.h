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

#ifndef EEPROM_ASYNC_H_
#define EEPROM_ASYNC_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>


/*PROTOTYPES-----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*eeprom_callback_t)(bool);


/**
 * Initializes EEPROM. After initiatization
 */
void eepromAsync_init();

// TODO enable disable instead of init, with matching timings


/**
 * Reads data from EEPROM in background job.
 *
 * @param addr 		byte address
 * @param data		pointer to store read data
 * @param length  	number of bytes to read
 * @param callback	callback after read is done (called from ISR)
 * @return 	true if job could be scheduled
 *
 * Callback is only executed if job could be scheduled.
 */
bool eepromAsync_read(uint32_t addr, uint8_t* data, uint16_t length, eeprom_callback_t callback);

/**
 * Writes data to EEPROM in background job.
 *
 * @addr 	byte address
 * @data	pointer to load data
 * @length  number of bytes to write
 * @param 	callback	callback after write is done (called from ISR)
 * @return 	true if job could be scheduled
 *
 * Callback is only executed if job could be scheduled.
 */
bool eepromAsync_write(uint32_t addr, const uint8_t* data, uint16_t length, eeprom_callback_t callback);





#ifdef __cplusplus
}
#endif

#endif /* EEPROM_ASYNC_H_ */
