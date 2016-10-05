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
#include "cfs.h"
#include "cfs-coffee.h"
#include "s25fl.h"
#include "s25fl_blocking.h"
#include "cometosError.h"
#include "cometos.h"

/****************************************************************************
* Name    :coffee_init

* Input   : None.
* Output  : None
* Inout   : None
* Returns : None
* Pre     :
* Post    :
* Descr   : Initialize the flash driver.
****************************************************************************/

uint8_t coffee_init(void)
{
    cometos_error_t result = -1;
    result = s25fl_init();
    return result;
}
/****************************************************************************
* Name    :coffee_write

* Input   : offset,
*           buf,
*           size.
* Output  : None
* Inout   : None
* Returns : None
* Pre     :
* Post    :
* Descr   : Invert all the bits before writing because, Coffee file system inverts
*           all read and written bits to have all data initialized to zero.
****************************************************************************/
void coffee_write(uint32_t offset,uint8_t *buf, uint16_t size)
{
    for(uint16_t i=0;i<size;i++) {
        buf[i] = ~(buf[i]);
    }

    //cometos::getCout() << "cw offset " << offset << " size " << size << cometos::endl;

    cometos_error_t result = s25fl_write(offset, buf, size);
    ASSERT(result == COMETOS_SUCCESS);

    for(uint16_t i=0;i<size;i++) {
        buf[i] = ~(buf[i]);
    }
}

/****************************************************************************
* Name    :coffee_read

* Input   : offset,
*           buf,
*           size.
* Output  : None
* Inout   : None
* Returns : None
* Pre     :
* Post    :
* Descr   : Invert all the bits before writing because, Coffee file system inverts
*           all read and written bits to have all data initialized to zero.
****************************************************************************/
void coffee_read(uint32_t offset,uint8_t *buf, uint16_t size)
{
    cometos_error_t result = s25fl_read(offset, buf, size);
    ASSERT(result == COMETOS_SUCCESS);

    for(uint16_t i=0;i<size;i++) {
        buf[i] = ~(buf[i]);
    }
}
/****************************************************************************
* Name    :coffee_erase

* Input   : sector.
* Output  : None
* Inout   : None
* Returns : None
* Pre     :
* Post    :
* Descr   : erase the given sector.
****************************************************************************/
void coffee_erase(uint32_t sector)
{
    cometos_error_t result = s25fl_sectorErase(sector*HAL_S25FL_SECTOR_SIZE);
    ASSERT(result == COMETOS_SUCCESS);
}

