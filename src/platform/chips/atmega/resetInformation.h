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

#ifndef RESETINFORMATION_H_
#define RESETINFORMATION_H_

// definitions used store/retrieve the PC of last function call
// into some predefined memory region. This memory region then
// can be read at startup

// TODO find a more portable way to define the memory regions, e.g. use
// sections and define linker symbols for their size and use those here
#define COMETOS_BOOT_PC *(uint32_t *) 0x800400
#define COMETOS_BOOT_MCUSR *(uint8_t *) 0x800404
#define COMETOS_BOOT_CHKSUM *(uint8_t *) 0x800405

#define COMETOS_BOOT_CHECK  ((uint8_t) \
                              ((*(uint8_t *) 0x800400)\
                             + (*(uint8_t *) 0x800401)\
                             + (*(uint8_t *) 0x800402)\
                             + (*(uint8_t *) 0x800403)\
                             + (*(uint8_t *) 0x800404)\
                             + (*(uint8_t *) 0x800405)))

#define COMETOS_BOOT_CALC_CHKSUM  (256 - ((uint8_t)\
                                  ((*(uint8_t *) 0x800400)\
                                 + (*(uint8_t *) 0x800401)\
                                 + (*(uint8_t *) 0x800402)\
                                 + (*(uint8_t *) 0x800403)\
                                 + (*(uint8_t *) 0x800404))))

#if ATMEGA_PC_WIDTH == 3
#define COMETOS_BOOT_GET_PC(pc)\
    uint8_t * stackPointer= ((uint8_t*)(SP))+1;\
    (pc) = (((uint32_t) (*(stackPointer) & 0x1)) << 17) | (((uint32_t) *(stackPointer+1)) << 9) | (((uint32_t) *(stackPointer+2)) << 1);
#else
#define COMETOS_BOOT_GET_PC(pc)\
    uint8_t * stackPointer= ((uint8_t*)(SP))+1;\
    (pc) = (((uint32_t) *(stackPointer)) << 9) | (((uint32_t) *(stackPointer+1)) << 1);
#endif

#endif /* RESETINFORMATION_H_ */
