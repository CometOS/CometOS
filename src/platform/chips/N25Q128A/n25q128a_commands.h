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

#ifndef N25Q128A_COMMANDS_H
#define N25Q128A_COMMANDS_H

#define N25Q128A_CMD_RDID     0x9E
#define N25Q128A_CMD_READ     0x03
#define N25Q128A_CMD_FASTREAD 0x0B
#define N25Q128A_CMD_ROTP     0x4B
#define N25Q128A_CMD_WREN     0x06
#define N25Q128A_CMD_WRDI     0x04
#define N25Q128A_CMD_PP       0x02
#define N25Q128A_CMD_POTP     0x42
#define N25Q128A_CMD_SSE      0x20
#define N25Q128A_CMD_SE       0xD8
#define N25Q128A_CMD_BE       0xC7
#define N25Q128A_CMD_PER      0x7A
#define N25Q128A_CMD_PES      0x75
#define N25Q128A_CMD_RDSR     0x05
#define N25Q128A_CMD_WRSR     0x01
#define N25Q128A_CMD_RDLR     0xE8
#define N25Q128A_CMD_WRLR     0xE5
#define N25Q128A_CMD_RFSR     0x70
#define N25Q128A_CMD_CLFSR    0x50
#define N25Q128A_CMD_RDNVCR   0xB5
#define N25Q128A_CMD_WRNVCR   0xB1
#define N25Q128A_CMD_RDVCR    0x85
#define N25Q128A_CMD_WRVCR    0x81
#define N25Q128A_CMD_RDVECR   0x65
#define N25Q128A_CMD_WRVECR   0x61

#endif

