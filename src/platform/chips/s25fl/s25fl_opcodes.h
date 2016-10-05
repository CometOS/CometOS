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

#ifndef S25FL_OPCODES_H
#define S25FL_OPCODES_H



enum {
    S25FL_OP_READ_STATUS,
    S25FL_OP_READ,
    S25FL_OP_WRITE,
    S25FL_OP_ERASE,
    S25FL_OP_OTHER,
};
typedef uint8_t s25fl_op_t;

enum {
    S25FL_CMD_RD_SR = 0x05, ///> read status register
    S25FL_CMD_RD_CR = 0x35,    ///> read config register
    S25FL_CMD_WR_SRCR = 0x01, ///> write status & cfg

    S25FL_CMD_READ = 0x03, ///> read data
    S25FL_CMD_WREN = 0x06, ///> write enable
    S25FL_CMD_WRDI = 0x04, ///> write disable

    S25FL_CMD_P4E = 0x20, ///> erase 4kb sector
    S25FL_CMD_P8E = 0x40, ///> erase 8kb sector
    S25FL_CMD_SE = 0xD8, ///> sector erase
    S25FL_CMD_BE = 0x60, ///> bulk erase

    S25FL_CMD_PP = 0x02, ///> program page

    S25FL_CMD_DP = 0xB9, ///> deep power-down
    S25FL_CMD_RES = 0xAB, ///> release from deep power-down
};
typedef uint8_t s25fl_cmd_t;





#endif
