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

#include "s25fl_async.h"
#include "palLed.h"
#include "palSpi.h"

#include "s25fl_pin.h"
#include "s25fl_opcodes.h"
#include "TaskScheduler.h"
#include "cometos.h"

using namespace cometos;

/** status register bits */
enum {
    WIP = 0,
    WEL = 1,
    BP0 = 2,
    BP1 = 3,
    BP2 = 4,
    E_ERR = 5,
    P_ERR = 6,
    SRWD = 7
};

///////////////////////////////////////////////////////////////////////////
/// "private" function declarations
///////////////////////////////////////////////////////////////////////////
static void pollStatus();

static void storeParams(s25fl_addr_t addr, uint8_t * rxbuf, const uint8_t * txbuf, s25fl_size_t len, s25fl_callBack cb);

/**
 * Sends a command to the Flash memory directly, i.e., without waiting for
 * the device to finish its current operation (should only be used for
 * reading the status register or after a non-device-busying operation
 */
static cometos_error_t syncSendCmd(s25fl_cmd_t cmdCode, s25fl_addr_t * addr, uint8_t numDummies, bool dataPending);

/**
 * Synchronous reading of a few bytes, e.g., to read config or status registers
 * of device. Must be used AFTER syncSendCmd with dataPending = true was called.
 * Will finish command sequence by pulling chip select high
 */
static cometos_error_t syncRead(uint8_t* rxbuf, s25fl_size_t lenData);

/**
 * Starts a data transmission (read/write), using the SPI. The task reschedules
 * itself until the transmission is done
 */
static void  dataIO();

static void callCb(cometos_error_t result);

///////////////////////////////////////////////////////////////////////////
/// "private" variables
///////////////////////////////////////////////////////////////////////////
static SimpleTask tPollStatus(pollStatus);
static SimpleTask tDataTransfer(dataIO);

static const s25fl_size_t MAX_BLOCK_SIZE = 16;

static s25fl_callBack currOpCb = EMPTY_CALLBACK();
static uint8_t* currRxBuf = NULL;
static const uint8_t* currTxBuf = NULL;
static s25fl_size_t currPos;
static s25fl_size_t currLen;
static s25fl_op_t   currOp;
static s25fl_cmd_t currCmdCode;
static s25fl_addr_t currAddr;


///////////////////////////////////////////////////////////////////////////
/// "private" function implementations
///////////////////////////////////////////////////////////////////////////
void storeParams(s25fl_addr_t addr, uint8_t* rxbuf, const uint8_t* txbuf, s25fl_size_t len, s25fl_callBack cb) {
    currAddr = addr;
    currTxBuf = txbuf;
    currRxBuf = rxbuf;
    currLen = len;
    currOpCb = cb;
    currPos = 0;
}

static void callCb(cometos_error_t result) {
    s25fl_callBack tmpCb = currOpCb;
    currOpCb = EMPTY_CALLBACK();
    currTxBuf = NULL;
    currRxBuf = NULL;
    tmpCb(result);
}

cometos_error_t readStatus(uint8_t * status) {
    cometos_error_t result = COMETOS_ERROR_FAIL;
    result = syncSendCmd(S25FL_CMD_RD_SR, NULL, 0, true);
    if (result != COMETOS_SUCCESS) {
        s25fl_deselect();
    } else {
        result = syncRead(status, 1);
    }
    return result;
}

cometos_error_t checkStatus() {
    uint8_t status = 0;
    cometos_error_t result = readStatus(&status);
    if (result != COMETOS_SUCCESS) {
        return COMETOS_ERROR_FAIL;
    } else if ((1<<WIP) & status) {
        return COMETOS_ERROR_BUSY;
    } else {
        return COMETOS_SUCCESS;
    }
}

static void pollStatus() {
    cometos_error_t result = checkStatus();
    if (result == COMETOS_ERROR_FAIL) {
        callCb(result);
        return;
    }

    // status register successfully read
    if (result == COMETOS_ERROR_BUSY) {
        // write/erase in progress, device busy, try again
        cometos::getScheduler().add(tPollStatus, 1);
    } else {
        result = COMETOS_SUCCESS;

        // dispatch action depending on operation
        switch(currOp) {
            case S25FL_OP_READ: {
                result = syncSendCmd(S25FL_CMD_READ, &currAddr, 0, true);
                if (result == COMETOS_SUCCESS) {
                    dataIO();
                } else {
                    s25fl_deselect();
                    callCb(result);
                }

            } break;
            case S25FL_OP_WRITE: {
                result = syncSendCmd(S25FL_CMD_WREN, NULL, 0, false);
                if (result == COMETOS_SUCCESS) {
                    result = syncSendCmd(S25FL_CMD_PP, &currAddr, 0, true);
                    if (result == COMETOS_SUCCESS) {
                        dataIO();
                    }  else {
                        s25fl_deselect();
                        callCb(result);
                    }
                }
            } break;
            case S25FL_OP_ERASE: {
                result = syncSendCmd(S25FL_CMD_WREN, NULL, 0, false);
                if (result == COMETOS_SUCCESS) {
                    result = syncSendCmd(S25FL_CMD_SE, &currAddr, 0, false);
                }
                callCb(result);
            } break;
            default: {
                callCb(COMETOS_ERROR_FAIL);
            }
        }
    }
}



static cometos_error_t syncSendCmd(s25fl_cmd_t cmdCode, s25fl_addr_t* addr, uint8_t numDummies, bool dataPending) {
    // check addr
    if (addr != NULL) {
        if (*addr >= HAL_S25FL_SIZE) {
            return COMETOS_ERROR_INVALID;
        }
    }

    uint8_t dummy;

    cometos_error_t result = palSpi_init(true, S25FL_SPI_FREQUENCY, 0, false);
    ASSERT(result == COMETOS_SUCCESS);

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
    if (!dataPending) {
        s25fl_deselect();
    }
    return COMETOS_SUCCESS;
}

static cometos_error_t syncRead(uint8_t* rxbuf, s25fl_size_t lenData) {
    if (rxbuf != NULL) {
        for (s25fl_size_t i = 0; i < lenData; i++) {
            palSpi_swapByte(rxbuf[i], rxbuf + i);
        }
    }
    s25fl_deselect();
    return COMETOS_SUCCESS;
}

static void dataIO(){
    ASSERT(currOp == S25FL_OP_WRITE || currOp == S25FL_OP_READ);

    s25fl_size_t toSend = currLen >= MAX_BLOCK_SIZE ? MAX_BLOCK_SIZE : currLen;

    cometos_error_t result = COMETOS_SUCCESS;

    // swap bytes depending on mode of operation
    // for reads, buffer is filled with read values,
    // for writes, content of buffer is send to device
    if (currOp == S25FL_OP_WRITE && currTxBuf != NULL) {
        result = palSpi_transmitBlocking(currTxBuf, NULL, toSend);
        currTxBuf+=toSend;
    } else if (currOp == S25FL_OP_READ && currRxBuf != NULL) {
        result = palSpi_transmitBlocking(NULL, currRxBuf, toSend);
        currRxBuf+=toSend;
    } else {
        ASSERT(false);
    }

    if (result != COMETOS_SUCCESS) {
        s25fl_deselect();
        callCb(result);
    }

    currLen-=toSend;

    // transmission finished?
    if (currLen == 0) {
        // we have finished sending, call operation_done and return.
        s25fl_deselect();
        callCb(COMETOS_SUCCESS);
    } else {
        cometos::getScheduler().add(tDataTransfer);
    }
}



///////////////////////////////////////////////////////////////////////////
/// public function implementations
///////////////////////////////////////////////////////////////////////////
//extern "C" cometos_error_t s25fl_readStatus(uint8_t * status) {
//    if (currOpCb != NULL) {
//        return COMETOS_ERROR_BUSY;
//    }
//    return readStatus(status);
//}



extern "C" cometos_error_t s25fl_readAsync(s25fl_addr_t addr, uint8_t * buf, s25fl_size_t len, s25fl_callBack cb) {
    if (addr >= HAL_S25FL_SIZE) {
        return COMETOS_ERROR_INVALID;
    }
    if (currOpCb) {
        return COMETOS_ERROR_BUSY;
    }

    storeParams(addr, buf, NULL, len, cb);

    currOp  = S25FL_OP_READ;
    currCmdCode = S25FL_CMD_READ;
    cometos::getScheduler().add(tPollStatus);
    return COMETOS_SUCCESS;
}


extern "C" cometos_error_t s25fl_pageProgramAsync(s25fl_addr_t addr, const uint8_t * buf, s25fl_size_t len, s25fl_callBack cb) {
    if (addr >= HAL_S25FL_SIZE || len > HAL_S25FL_PAGE_SIZE) {
        return COMETOS_ERROR_INVALID;
    }
    if (currOpCb) {
        return COMETOS_ERROR_BUSY;
    }

    storeParams(addr, NULL, buf, len, cb);

    currOp = S25FL_OP_WRITE;
    currCmdCode = S25FL_CMD_PP;

    cometos::getScheduler().add(tPollStatus);
    return COMETOS_SUCCESS;
}



extern "C" cometos_error_t s25fl_sectorEraseAsync(s25fl_addr_t addr, s25fl_callBack cb) {
    if (addr >= HAL_S25FL_SIZE) {
        return COMETOS_ERROR_INVALID;
    }
    if (currOpCb) {
        return COMETOS_ERROR_BUSY;
    }

    storeParams(addr, NULL, NULL, 0, cb);
    currOp = S25FL_OP_ERASE;
    currCmdCode = S25FL_CMD_SE;

    cometos::getScheduler().add(tPollStatus);
    return COMETOS_SUCCESS;
}
