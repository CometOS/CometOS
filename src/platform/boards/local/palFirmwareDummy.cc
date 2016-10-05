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

#include <iostream>
#include "palFirmware.h"
#include "logging.h"
#include "firmwareVersion.h"

using namespace std;



palFirmware_ret_t palFirmware_init() {
	cout<<"Inititalize  fake firmware update abstraction layer"<<endl;
	return PAL_FIRMWARE_SUCCESS;
}




bool palFirmware_isValid(uint8_t slot) {
	return false;
}


bool palFirmware_isFinal(uint8_t slot) {
	return false;
}

uint8_t palFirmware_getNumSlots() {
	return 0;
}


uint16_t palFirmware_getSlotSize() {
	return 0;
}




palFirmware_ret_t palFirmware_erase(uint8_t slot){
	return PAL_FIRMWARE_NOT_SUPPORTED;
}



palFirmware_ret_t palFirmware_write(const uint8_t *data,uint8_t slot, uint16_t segIndex) {
	return PAL_FIRMWARE_NOT_SUPPORTED;
}


palFirmware_ret_t palFirmware_read(uint8_t *data, uint8_t slot, uint16_t segIndex) {
	return PAL_FIRMWARE_NOT_SUPPORTED;
}


palFirmware_ret_t palFirmware_validate(uint8_t slot, uint32_t address_offset, uint16_t crc) {
	return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_eraseAsync(uint8_t slot, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_writeAsync(const uint8_t *data,uint8_t slot, uint16_t segIndex, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_readAsync(uint8_t *data, uint8_t slot, uint16_t segIndex, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_validateAsync(uint8_t slot, uint32_t address_offset, uint16_t crc, palFirmware_callback_t callback) {
    return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_run(uint8_t slot) {
	return PAL_FIRMWARE_NOT_SUPPORTED;

}

palFirmware_ret_t palExec_reset() {
	return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_initTransferAsync(palFirmware_slotNum_t slot,
                                                palFirmware_segNum_t numSegs,
                                                palFirmware_callback_t callback,
                                                palFirmware_crc_t crc) {
	return PAL_FIRMWARE_NOT_SUPPORTED;
}

palFirmware_ret_t palFirmware_validateAsync(palFirmware_slotNum_t slot, palFirmware_callback_t callback) {
	return PAL_FIRMWARE_NOT_SUPPORTED;
}

firmwareVersionNumber_t firmware_getVersionNumber() {
    return 0;
}
firmwareTimestamp_t firmware_getTimestamp() {
    return 0;
}

firmwareVersion_t firmware_getVersion() {
    return 0;
}

