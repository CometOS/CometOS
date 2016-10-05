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

#ifndef PALFIRMWAREDEFS_H_
#define PALFIRMWAREDEFS_H_

#ifndef P_FIRMWARE_NUM_SEGS_IN_STORAGE
// use 256k, that is 4 sectors of s25fl flash mem for firmware (ROM is 256k)
#define P_FIRMWARE_NUM_SEGS_IN_STORAGE	1024
#endif
#ifndef P_FIRMWARE_NUM_SLOTS
#define P_FIRMWARE_NUM_SLOTS	1
#endif

#ifndef P_FIRMWARE_NUM_SEGS_IN_ROM
// used by bootloader to copy firmware into ROM, reduce size by 32 segments
// which are used for bootloader
#define P_FIRMWARE_NUM_SEGS_IN_ROM 992
#endif

#define FIRMWARE_OFFSET 0xC000

#define PAL_PERS_ADDRESS 0x000FE000 // TODO make dynamic
#define PAL_PERS_SIZE 0x100 // can be larger (0x1000), but is not needed
#define PAL_FIRMWARE_ADDRESS 0x000BE000 // TODO make dynamic
#define PAL_FIRMWARE_LENGTH  0x00040000 // TODO make dynamic
#define PAL_FIRMWARE_SECTOR_SIZE 0x00001000 // TODO correct?

#endif /* PALFIRMWAREDEFS_H_ */
