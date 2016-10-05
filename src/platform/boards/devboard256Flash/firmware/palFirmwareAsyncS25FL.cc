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

#include "palFirmware.h"
#include "palFirmwareDefs.h"
#include "s25fl_async.h"
//#include <avr/wdt.h>


#if ((P_FIRMWARE_SEGMENT_SIZE * P_FIRMWARE_NUM_SEGS_IN_STORAGE) % HAL_S25FL_SECTOR_SIZE) != 0
#error "Firmware size has to be multiple of S25FL flash memory sector size, i.e., of 65536 bytes"
#endif

#if (P_FIRMWARE_SEGMENT_SIZE != HAL_S25FL_PAGE_SIZE)
#error "Segment size has to agree with page size of flash memory (256 bytes)"
#endif

#if (P_FIRMWARE_SEGMENT_SIZE * P_FIRMWARE_NUM_SEGS_IN_STORAGE * P_FIRMWARE_NUM_SLOTS > HAL_S25FL_SIZE)
#error "Memory reserved for firmware is larger than target flash memory"
#endif

const uint8_t FIRMWARE_S25FL_SECTORS = (((uint32_t)P_FIRMWARE_SEGMENT_SIZE) * P_FIRMWARE_NUM_SEGS_IN_STORAGE) / HAL_S25FL_SECTOR_SIZE;

//static palFirmware_callback_t currCallback = EMPTY_CALLBACK();

using namespace cometos;

palFirmware_ret_t palFirmwareImpl_init() {
  cometos_error_t result = s25fl_init();
  if (result != COMETOS_SUCCESS) {
    return PAL_FIRMWARE_ERROR;
  }
  return PAL_FIRMWARE_SUCCESS;
}

uint8_t palFirmwareImpl_getNumSectors() {
  return FIRMWARE_S25FL_SECTORS;
}

uint32_t palFirmwareImpl_getSectorSize() {
  return HAL_S25FL_SECTOR_SIZE;
}

cometos_error_t palFirmwareImpl_sectorEraseAsync(uint32_t addr, Callback<void(cometos_error_t)> callback) {
  return s25fl_sectorEraseAsync(addr, callback);
}

cometos_error_t palFirmwareImpl_pageProgramAsync(uint32_t addr, const uint8_t * buf, uint16_t len, cometos::Callback<void(cometos_error_t)> callback) {
  return s25fl_pageProgramAsync(addr, buf, len, callback);
}

cometos_error_t palFirmwareImpl_readAsync(uint32_t addr, uint8_t * buf, uint16_t len, cometos::Callback<void(cometos_error_t)> cb) {
  return s25fl_readAsync(addr, buf, len, cb);
}
