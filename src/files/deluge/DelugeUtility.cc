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

#include "DelugeUtility.h"

using namespace cometos;

uint8_t DelugeUtility::GetLeastSignificantBitSet(uint32_t value) {
    uint8_t packetIndex = DELUGE_UINT8_OUT_OF_RAGE;
    for (uint32_t i = 0; i < 32; i++) {
        if (((value>>i)&(uint32_t)1) != 0) {
            packetIndex = i;
            break;
        }
    }
    return packetIndex;
}

void DelugeUtility::UnsetBit(uint32_t *value, uint8_t bit) {
    *value &= ~(((uint32_t)1) << (uint32_t)bit);
}

void DelugeUtility::SetBit(uint32_t *value, uint8_t bit) {
    *value |= 1 << (uint32_t)bit;
}

void DelugeUtility::SetFirstBits(uint32_t *value, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        DelugeUtility::SetBit(value, i);
    }
}

uint16_t DelugeUtility::NumOfPacketsInPage(uint8_t page, uint32_t fileSize) {
    uint32_t offset = (uint32_t)page * DELUGE_PAGE_SIZE;
    ASSERT(fileSize >= offset);
    uint32_t pageSize = fileSize - offset;
    if (pageSize > DELUGE_PAGE_SIZE)
        pageSize = DELUGE_PAGE_SIZE;
    return (pageSize / DELUGE_PACKET_SEGMENT_SIZE) + ((pageSize % DELUGE_PACKET_SEGMENT_SIZE != 0)?1:0);
}

//TODO
uint16_t DelugeUtility::PacketSize(uint8_t page, uint8_t packet, uint32_t fileSize) {
    uint32_t offset = (uint32_t)page * DELUGE_PAGE_SIZE + packet * DELUGE_PACKET_SEGMENT_SIZE;
    ASSERT(fileSize >= offset);
    return (fileSize - offset > DELUGE_PACKET_SEGMENT_SIZE)?DELUGE_PACKET_SEGMENT_SIZE:(fileSize - offset);
}








