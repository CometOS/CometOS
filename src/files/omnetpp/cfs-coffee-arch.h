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

#ifndef CFS_COFFEE_ARCH_H
#define CFS_COFFEE_ARCH_H

/*INCLUDES-------------------------------------------------------------------*/
#include <stdint.h>

#if 0
/* Coffee configuration parameters. */
#define COFFEE_SECTOR_SIZE		65536UL
#define COFFEE_PAGE_SIZE		256UL

/* Byte page size, starting address on page boundary, and size of the file system */
#define COFFEE_ADDRESS            0x200000
#define COFFEE_START              (COFFEE_ADDRESS & ~(COFFEE_PAGE_SIZE-1))

#define COFFEE_SIZE			(1024UL * 1024UL * 2)
#define COFFEE_NAME_LENGTH		16
#define COFFEE_MAX_OPEN_FILES		6
#define COFFEE_FD_SET_SIZE		8
#define COFFEE_LOG_TABLE_LIMIT		256
#ifdef COFFEE_CONF_DYN_SIZE
#define COFFEE_DYN_SIZE			COFFEE_CONF_DYN_SIZE
#else
#define COFFEE_DYN_SIZE     4*1024
#endif
#define COFFEE_LOG_SIZE			1024

#define COFFEE_IO_SEMANTICS		1
#define COFFEE_APPEND_ONLY		0
#define COFFEE_MICRO_LOGS		1

/* Flash operations. */

void coffee_write(uint32_t offset,uint8_t *buf, uint16_t size);
void coffee_read(uint32_t offset,uint8_t *buf, uint16_t size);
void coffee_erase(uint32_t sector);
uint8_t coffee_init(void);

#define COFFEE_WRITE(buf, size, offset) \
        coffee_write((uint32_t)offset,(uint8_t *)buf, (uint16_t) size)

#define COFFEE_READ(buf, size, offset) \
        coffee_read((uint32_t)offset,(uint8_t *)buf, (uint16_t) size)

#define COFFEE_ERASE(sector) coffee_erase((uint32_t)sector)

#endif

/* Coffee types. */
typedef int32_t coffee_page_t; // not necessary, but it is not always correctly casted when converting it to offset
typedef int32_t cfs_offset_t;

#endif /* !COFFEE_ARCH_H */
