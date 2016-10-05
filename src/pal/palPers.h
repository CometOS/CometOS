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
 * CometOS Platform Abstraction Layer for Persistence.
 *
 * @author Stefan Unterschuetz
 */
#ifndef PALPERS_H_
#define PALPERS_H_

#include "types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initializes module.
 */
void palPers_init();

/**
 * Persistently writes data to non-volatile memory.
 *
 * @param addr		start address
 * @param buffer	pointer to buffer
 * @param length	length of buffer
 */
void palPers_write(uint16_t addr, const uint8_t* buffer, uint8_t length);


/**
 * Reads data from a non-volatile memory.
 *
 * @param addr		start address
 * @param buffer	pointer to buffer
 * @param length	length of buffer
 */
void palPers_read(uint16_t addr, uint8_t* buffer, uint8_t length);

/**
 * Reads a single bit from non-volatile memory.
 *
 * @param addr address of the byte to read from
 * @param i    bit within the byte to evaluate (0-7)
 * @return     TRUE, if bit is set
 *             FALSE else
 */
bool palPers_getBit(uint16_t address, uint8_t i);

/**
 * Set/clear a single bit in non-volatile memory
 *
 * @param value   value of the bit
 * @param address address of the byte to write to
 * @param i       bit within the byte to set or clear, according to value (0-7)
 */
void palPers_setBit(bool value, uint16_t address, uint8_t i);

#ifdef __cplusplus
}
#endif


#endif /* PALPERS_H_ */
