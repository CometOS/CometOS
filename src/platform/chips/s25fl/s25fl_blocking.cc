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

#include "s25fl_blocking.h"
#include "s25fl_opcodes.h"
#include "palLed.h"
#include "palSpi.h"

#include "s25fl_pin.h"

// We do NOT yet support 4 byte addressing modes, e.g., for
// devices > 256 Mebi bits


///////////////////////////////////////////////////////////////////////////
/// "private" function declarations
///////////////////////////////////////////////////////////////////////////

/*
 * Busily waits for the device to be idle and then sends the given command
 */
static cometos_error_t waitAndSendCmd(s25fl_op_t op,
                               s25fl_cmd_t cmdCode,
                               uint8_t numDummies,
                               s25fl_addr_t * addr,
                               uint8_t* rxbuf,
                               const uint8_t* txbuf,
                               s25fl_size_t lenData);

/*
 * Sends a command to the Flash memory directly, i.e., without waiting for
 * the device to finish its current operation (should only be used for
 * reading the status register or after a non-device-busying operation
 */
static cometos_error_t sendCmd(s25fl_op_t op,
                               s25fl_cmd_t cmdCode,
                               uint8_t numDummies,
                               const s25fl_addr_t* addr,
                               uint8_t* rxbuf,
                               const uint8_t* txbuf,
                               s25fl_size_t lenData);


static cometos_error_t waitAndSendCmd(s25fl_op_t op,
                               s25fl_cmd_t cmdCode,
                               uint8_t numDummies,
                               s25fl_addr_t * addr,
                               uint8_t * rxbuf,
                               const uint8_t * txbuf,
                               s25fl_size_t lenData) {

    // wait for idle state
    bool busy = true;
    while(busy) {
        uint8_t status = 0;
        sendCmd(S25FL_OP_READ, S25FL_CMD_RD_SR, 0, NULL, &status, NULL, 1);
        busy = (0x01 & status);
    }
    return sendCmd(op, cmdCode, numDummies, addr, rxbuf, txbuf, lenData);
}


/*
 * Generic function to send every possible type of command (using address
 * bytes, using dummy bytes, using data cycles etc.) to the flash memory.
 *
 * @param op distinguishes commands regarding their data cycles:
 *           S25FL_OP_OTHER: no data cycle
 *           S25FL_OP_READ:  lenData data cycles, reply from device is stored
 *                           in buf
 *           S25FL_OP_WRITE: lenData data cycles, data in buf is send to device
 * @param cmdCode command code of the command to be executed
 * @param numDummies number of dummy bytes required by the command
 * @param addr pointer to the address or NULL, if no address is used by command
 * @param buf  pointer to the read/write buffer
 * @param lenData length of data to be read or written, must correspond to the
 *                memory pointed at by buf
 */
static cometos_error_t sendCmd(s25fl_op_t op,
                               s25fl_cmd_t cmdCode,
                               uint8_t numDummies,
                               const s25fl_addr_t * addr,
                               uint8_t * rxbuf,
                               const uint8_t * txbuf,
                               s25fl_size_t lenData){

    // check if operation type matches data len
    if (op == S25FL_OP_OTHER && lenData > 0) {
        return COMETOS_ERROR_INVALID;
    }
    // check addr
    if (addr != NULL) {
        if (*addr >= HAL_S25FL_SIZE) {
            return COMETOS_ERROR_INVALID;
        }
    }

    uint8_t dummy;
    cometos_error_t result = COMETOS_SUCCESS;

    cometos_error_t ret = palSpi_init(true, S25FL_SPI_FREQUENCY, 0, false);
    if(ret != COMETOS_SUCCESS) {
	    return ret;
    }

    // select chip
    s25fl_select();
    palSpi_swapByte(cmdCode, &dummy);

    // send dummy bytes if command requires it
    for (uint8_t i=0; i < numDummies; i++) {
        palSpi_swapByte(dummy, &dummy);
    }

    // send address bytes if addr is set
    if (addr != NULL) {
        s25fl_addr_t tmpAddr = *addr;
        palSpi_swapByte((tmpAddr >> 16) & 0xFF, &dummy);
        palSpi_swapByte((tmpAddr >>  8) & 0xFF, &dummy);
        palSpi_swapByte(tmpAddr & 0xFF, &dummy);
    }

    // swap bytes depending on mode of operation
    // for reads, buffer is filled with read values,
    // for writes, content of buffer is send to device
    s25fl_size_t i;
    if (op == S25FL_OP_WRITE && txbuf != NULL) {
        for (i = 0; i < lenData; i++) {
            palSpi_swapByte(txbuf[i], &dummy);
        }
    } else if (op == S25FL_OP_READ && rxbuf != NULL) {
        for (i = 0; i < lenData; i++) {
            palSpi_swapByte(dummy, &rxbuf[i]);
        }
    } else {
        if (lenData != 0) {
            result = COMETOS_ERROR_INVALID;
        }
    }

    s25fl_deselect();
    return result;
}




//cometos_error_t s25fl_readStatus(uint8_t * status) {
//    return sendCmd(S25FL_OP_READ, S25FL_CMD_RD_SR, 0, NULL, status, NULL, 1);
//}


cometos_error_t s25fl_read(uint32_t addr, uint8_t * buf, uint16_t len) {
    if (addr + len >= HAL_S25FL_SIZE) {
        return COMETOS_ERROR_INVALID;
    }
    
    return waitAndSendCmd(S25FL_OP_READ, S25FL_CMD_READ, 0, &addr, buf, NULL, len);;
}


cometos_error_t s25fl_pageProgram(uint32_t addr, const uint8_t * buf, uint16_t len) {
    if (addr >= HAL_S25FL_SIZE || len > HAL_S25FL_PAGE_SIZE || addr % HAL_S25FL_PAGE_SIZE != 0) {
        return COMETOS_ERROR_INVALID;
    }

    // enable write mode
    waitAndSendCmd(S25FL_OP_OTHER, S25FL_CMD_WREN, 0, NULL, NULL, NULL, 0);
    return sendCmd(S25FL_OP_WRITE, S25FL_CMD_PP, 0, &addr, NULL, buf, len);;
}

cometos_error_t s25fl_write(uint32_t addr, const uint8_t * buf, uint16_t len) {
    if (addr+len >= HAL_S25FL_SIZE) {
        return COMETOS_ERROR_INVALID;
    }

    /**
     * Example
     * page size = 4
     * addr = 1
     * len = 8
     *
     * 0: 0 1 2 3
     * 1: 4 5 6 7
     * 2: 8 9 A B
     */

    uint32_t firstPage = addr / HAL_S25FL_PAGE_SIZE;            // = 0
    uint32_t finalAddr = addr + len - 1;                        // = 1 + 8 - 1 = 8
    uint32_t finalPage = finalAddr / HAL_S25FL_PAGE_SIZE;       // = 8 / 4 = 2

    for(uint32_t page = firstPage; page <= finalPage; page++) {
        // enable write mode
        waitAndSendCmd(S25FL_OP_OTHER, S25FL_CMD_WREN, 0, NULL, NULL, NULL, 0);

        // calculate address and length
        uint32_t currAddr = page*HAL_S25FL_PAGE_SIZE;           // = {0,4,8}
        uint16_t currLen = HAL_S25FL_PAGE_SIZE;                 // = 4

        if(page == firstPage) {
            // first page
            currLen = HAL_S25FL_PAGE_SIZE - (addr - currAddr);  // = 4 - (1 - 0) = 3
            currAddr = addr;                                    // = 1
        }

        if(page == finalPage) { // can also be at the same time as the first page
            currLen = finalAddr - currAddr + 1;                 // = 8 - 8 + 1 = 1
        }

        // do the write
        cometos_error_t result = sendCmd(S25FL_OP_WRITE, S25FL_CMD_PP, 0, &currAddr, NULL, buf, currLen);
        if(result != COMETOS_SUCCESS) {
            return result;
        }

        // increase the buffer pointer
        buf += currLen;
    }

    return COMETOS_SUCCESS;
}

cometos_error_t s25fl_sectorErase(uint32_t addr) {
    if (addr >= HAL_S25FL_SIZE) {
        return COMETOS_ERROR_INVALID;
    }

    waitAndSendCmd(S25FL_OP_OTHER, S25FL_CMD_WREN, 0, NULL, NULL, NULL, 0);
    return sendCmd(S25FL_OP_OTHER, S25FL_CMD_SE, 0, &addr, NULL, NULL, 0);
}
