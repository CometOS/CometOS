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

#include "OtapBase.h"
#include "primitives.h"
#include "OutputStream.h"

namespace cometos {

const char* const OtapBase::OTAP_MODULE_NAME = "otap";
const uint8_t OtapBase::NO_CURR_SLOT = 0xFF;

OtapBase::OtapBase(const char* name):
        OtapBlockTransfer<OTAP_NUM_PKTS, OTAP_NUM_RED_PKTS,
        OTAP_PKT_SIZE>(name),
        currSlot(OtapBase::NO_CURR_SLOT) {
}

void OtapBase::initialize() {
    palFirmware_init();
    remoteDeclare(&OtapBase::run, "run");
    remoteDeclare(&OtapBase::getNumMissingVectors, "gnmv");
    remoteDeclare(&OtapBase::getMissingVector, "gmv");
    remoteDeclare(&OtapBlockTransferBase::setIntervalRemote, "si");
}

void OtapBase::runFirmware(OtapRunMessage* msg) {
    palFirmware_run(msg->slot);
    delete msg;

}

uint8_t OtapBase::run(uint8_t& slot, uint16_t& delay) {
    if (!palFirmware_isValid(slot)) {
        return PAL_FIRMWARE_INVALID_FIRMWARE;
    }

    OtapRunMessage* msg = new OtapRunMessage(slot);
    schedule(msg, &OtapBase::runFirmware, delay);
    return PAL_FIRMWARE_SUCCESS;
}

BitVector<OTAP_BITVECTOR_SIZE> OtapBase::getMissingVector(uint8_t & num) {
    if (num >= OTAP_NUM_BITVECTORS) {
        ASSERT(false);
        return BitVector<OTAP_BITVECTOR_SIZE>();
    } else {
        return receivedSegments.getVector(num);
    }
}

uint8_t OtapBase::getNumMissingVectors() {
    return OTAP_NUM_BITVECTORS;
}


palFirmware_ret_t OtapBase::checkInit(palFirmware_slotNum_t slot, palFirmware_segNum_t seg) {
//    getCout() << (int) slot << "|" << seg << "|" << palFirmware_getSlotSize() << "|" << cometos::endl;
    if (palFirmware_isFinal(slot)) {
        return PAL_FIRMWARE_IS_FINAL;
    }
    if (slot >= palFirmware_getNumSlots()) {
        return PAL_FIRMWARE_INVALID_SLOT;
    }
    if (seg == 0
            || seg > P_FIRMWARE_NUM_SEGS_IN_STORAGE
            || seg > palFirmware_getSlotSize()) {
        return PAL_FIRMWARE_SIZE_ERROR;
    }
    return PAL_FIRMWARE_SUCCESS;
}


palFirmware_slotNum_t OtapBase::getCurrSlot() {
    return currSlot;
}

void OtapBase::setCurrSlot(palFirmware_slotNum_t currSlot) {
    if (currSlot < palFirmware_getNumSlots()){
        this->currSlot = currSlot;
    }
}

palFirmware_segNum_t OtapBase::getSegCount() {
    return segCount;
}

void OtapBase::setSegCount(palFirmware_segNum_t numSeg) {
    if (numSeg <= P_FIRMWARE_NUM_SEGS_IN_STORAGE && numSeg <= palFirmware_getSlotSize()) {
        segCount = numSeg;
    }
}

BitVectorWrapper<P_FIRMWARE_NUM_SEGS_IN_STORAGE, OTAP_BITVECTOR_SIZE>& OtapBase::getReceivedSegments() {
    return receivedSegments;
}

void serialize(ByteVector& buffer, const OtapInitMessage & value) {
    serialize(buffer, value.slot);
    serialize(buffer, value.segCount);
    serialize(buffer, value.crc);
}

void unserialize(ByteVector& buffer, OtapInitMessage & value) {
    unserialize(buffer, value.crc);
    unserialize(buffer, value.segCount);
    unserialize(buffer, value.slot);
}


}
