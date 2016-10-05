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

#include "OtapAsync.h"

namespace cometos {

OtapAsync* OtapAsync::instance = NULL;

const char* OtapAsync::EVENT_DONE_NAME = "oed";

OtapAsync::OtapAsync() :
        OtapBase(OtapBase::OTAP_MODULE_NAME),
        eventDone(this, OtapAsync::EVENT_DONE_NAME)
{
    ASSERT(instance == NULL);
    instance = this;
}

OtapAsync::~OtapAsync()
{}


void OtapAsync::initialize() {
    OtapBase::initialize();
    remoteDeclare(&OtapAsync::initiate, "init", OtapAsync::EVENT_DONE_NAME);
    remoteDeclare(&OtapAsync::verify, "veri", OtapAsync::EVENT_DONE_NAME);
}

void OtapAsync::eraseDone(palFirmware_ret_t result) {
    instance->setSegCount(instance->tmpSegNum);
    instance->setCurrSlot(instance->tmpSlot);
    instance->getReceivedSegments().fill(false);
    instance->eventData.opId = OTAP_ERASE;
    instance->eventData.status = result;
    instance->eventData.size = instance->tmpSegNum;
    instance->eventData.version = 0;
    instance->state = OTAP_IDLE;
    instance->eventDone.raiseEvent(instance->eventData);
}

uint8_t OtapAsync::initiate(OtapInitMessage & msg) {
    palFirmware_ret_t ret = checkInit(msg.slot, msg.segCount);

    if (ret != PAL_FIRMWARE_SUCCESS) {
        return ret;
    }

    tmpSegNum = msg.segCount;
    tmpSlot = msg.slot;

    if (state != OTAP_IDLE) {
        return PAL_FIRMWARE_BUSY;
    }

    state = OTAP_ERASE;

    return palFirmware_initTransferAsync(msg.slot, msg.segCount, CALLBACK_FUN(OtapAsync::eraseDone), msg.crc);
}

void OtapAsync::writeDone(palFirmware_ret_t result) {
    if (result == PAL_FIRMWARE_SUCCESS) {
        instance->getReceivedSegments().set(instance->tmpSegNum, true);
    }
    instance->state = OTAP_IDLE;
}

void OtapAsync::recvSegment(uint8_t * data, uint16_t segId) {
    if (getCurrSlot() == OtapBase::NO_CURR_SLOT) {
        return;
    }

    if (state != OTAP_IDLE) {
        return;
    }

    state = OTAP_WRITE;
    tmpSegNum = segId;

    palFirmware_writeAsync(data, getCurrSlot(), segId, CALLBACK_FUN(OtapAsync::writeDone));
}


void OtapAsync::verifyDone(palFirmware_ret_t result) {
    instance->eventData.opId = OTAP_VERIFY;
    instance->eventData.status = result;
    instance->eventData.size = 0;
    instance->eventData.version = 0;
    instance->state = OTAP_IDLE;
    instance->eventDone.raiseEvent(instance->eventData);
}

uint8_t OtapAsync::verify(palFirmware_slotNum_t & slot) {
    if (state != OTAP_IDLE) {
        return PAL_FIRMWARE_BUSY;
    }

    // we just pass the task of verifying to the firmware layer
    return palFirmware_validateAsync(slot, CALLBACK_FUN(OtapAsync::verifyDone));
}

} /* namespace cometos */
