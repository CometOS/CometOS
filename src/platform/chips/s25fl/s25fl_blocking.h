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
 * CometOS Hardware Abstraction Layer for flash memory S25FL
 *
 * @author Andreas Weigel
 */

#ifndef HAL_S25FL_BLOCKING_H
#define HAL_S25FL_BLOCKING_H

#include "cometosError.h"
#include "s25fl.h"
#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif



/**
 * Reads len bytes from the specified address into buf. Blocking.
 *
 * @param buf  pointer to buffer to store the data
 * @param addr address of the data to be read
 * @param len  number of bytes to be read from addr
 * @return COMETOS_SUCCESS if data was read successfully
 *         ...
 */
cometos_error_t s25fl_read(s25fl_addr_t addr, uint8_t * buf, s25fl_size_t len);

/**
 * Reads the status register of the device
 */
cometos_error_t s25fl_readStatus(uint8_t * status);

/**
 * Writes len bytes from buf into page at addr. Can ony be used to program
 * bytes from 1 to 0 or let them stay at value of 0. Bytes can only be
 * set back to 1 by an erase command. This function does not check whether
 * a page was programmed early and if the content afterwards matches the
 * given input.
 *
 * If address is not aligned to a page and len would reach into the next page,
 * the beginning of the page is programmed instead, i.e. you can always only
 * program ONE page.
 *
 * @param buf  buf holding the data to be written
 * @param addr address of the memory to be programmed
 * @param len  number of bytes to be written
 * @return ...
 */
cometos_error_t s25fl_pageProgram(s25fl_addr_t addr, const uint8_t * buf, s25fl_size_t len);

/**
 * Erases the sector containing the given address. All bits in this sector
 * are un-programmed, i.e. set to 1.
 *
 * @param addr address within the sector which is to be erased
 */
cometos_error_t s25fl_sectorErase(s25fl_addr_t addr);


cometos_error_t s25fl_write(s25fl_addr_t addr, const uint8_t * buf, s25fl_size_t len);

#ifdef __cplusplus
}
#endif

#endif /* HAL_S25FL_BLOCKING_H */
