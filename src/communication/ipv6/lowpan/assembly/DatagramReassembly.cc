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

/*
 * @author Andreas Weigel
 */

#include "DatagramReassembly.h"
#include "lowpan-macros.h"
#include "logging.h"

namespace cometos_v6 {

const uint8_t DatagramReassembly::OCCUPIED_ARRAY_SIZE;

void DatagramReassembly::free() {
    if (dcDatagram != NULL) {
        delete dcDatagram;
    }
    if (buffer != NULL) {
        buffer->free();
    }
    buffer = NULL;
    dcDatagram = NULL;
    ticksLeft = ASSEMBLYBUFFER_STATUS_FREE;
    rxInfo.clear();
    uncompressedOffset = 0;
}

void DatagramReassembly::initialize(const Ieee802154MacAddress& srcMAC,
                                     BufferInformation* buf,
                                     uint16_t tag,
                                     uint16_t size)
{
    // first cleanup, if there's something not cleaned up
    free();

    // now initialize to given values
    this->MACAddr = srcMAC;
    this->buffer = buf;
    clearOccupied();
    clearAckBitMap();
    ticksLeft = ASSEMBLYBUFFER_STATUS_RECEIVED;
    this->size = size;
    this->tag = tag;
    LOG_DEBUG("initialized fi " << cometos::hex << (uintptr_t) this
              << cometos::dec << ": size=" << size
              << "|tag=" << tag
              << cometos::hex << "|buf=" << (uintptr_t) buf << cometos::dec);
}

void DatagramReassembly::setDecompressionInfo(uint16_t uncompressedBufferPos,
                          uint8_t uncompressedOffset) {
    this->uncompressedOffset = uncompressedOffset;
    this->uncompressedBufferPos = uncompressedBufferPos;
}



uint16_t DatagramReassembly::getContiguousSize() const {
    uint16_t csize = 0;
    uint8_t i = 0;
    uint16_t j = 0x8000;
    while ((i < 16) && (occupied[i]&j)) {
        csize += 8;
        j = j >> 1;
        if (j == 0) {
            i++;
            j = 0x8000;
        }
    }
    uint16_t ret = csize;
    return ret;
}

void DatagramReassembly::updateLlRxInfo(const cometos::Airframe & frame) {
    if (frame.has<cometos::MacRxInfo>()) {
        const cometos::MacRxInfo * info = frame.get<cometos::MacRxInfo>();
        rxInfo.newFrame(info->rssi, info->lqi, info->lqiIsValid);
    }
}

inline void DatagramReassembly::nextBitInBitMap(
        uint8_t & bitmapArrayIndex,
        uint16_t & bitmapBitMask) {
    bitmapBitMask = bitmapBitMask >> 1;
    if (bitmapBitMask == 0) {
        bitmapArrayIndex++;
        bitmapBitMask = 0x8000;
    }
    ASSERT(bitmapArrayIndex < OCCUPIED_ARRAY_SIZE);
}

void DatagramReassembly::clearOccupied() {
   uncompressedBufferPos = 0;
   for (uint8_t i = 0; i < 16; i++) occupied[i] = 0;
}

void DatagramReassembly::clearAckBitMap(){
   acknowledgementBitmap = 0;
}

bool DatagramReassembly::addFragment(
            uint8_t lowpanFragOffset,
            cometos::Airframe & frame, uint16_t uncompressedSize) {
    if (!((uncompressedSize & LOWPAN_FRAG_OFFSET_MASK) == 0
            || (offsetToByteSize(lowpanFragOffset) + uncompressedSize == size))) {
        return false;
    }
    if (ticksLeft == ASSEMBLYBUFFER_STATUS_FREE ||
            ticksLeft == ASSEMBLYBUFFER_STATUS_DONE) return false;

    uint8_t  bitmapArrayIndex    = lowpanFragOffset >> 4; //< index to the element of the occupied array
    uint16_t bitmapBitMask    = 0x8000 >> (lowpanFragOffset&0xF); //< bitmask within the element

    // check if fragment for offset has been already stored
    if ((occupied[bitmapArrayIndex] & bitmapBitMask) == 0) {
        updateLlRxInfo(frame);
        ticksLeft = ASSEMBLYBUFFER_STATUS_RECEIVED;

        // update the occupied array, corresponding to uncompressed size
        for (uint8_t i = 0; i < (uncompressedSize >> 3); i++) {
            occupied[bitmapArrayIndex] |= bitmapBitMask;
            bitmapBitMask = bitmapBitMask >> 1;
            if (bitmapBitMask == 0) {
                bitmapArrayIndex++;
                bitmapBitMask = 0x8000;
            }
        }

        // in case this is the last packet, we additionally mark the last
        // bit as occupied, although -- strictly speaking -- not all of its
        // bytes are present
        if (uncompressedSize & LOWPAN_FRAG_OFFSET_MASK) {
            occupied[bitmapArrayIndex] |= bitmapBitMask;
        }

        uint16_t csize = getContiguousSize();
        if (csize >= size) {
            ticksLeft = ASSEMBLYBUFFER_STATUS_DONE;
        }

        LOG_DEBUG("Added frame of len " << (int) frame.getLength()
                << " to buffer; continuousSize=" << csize
                << "|size=" << (int) size);

        // if this is the first fragment, directly use the stored
        // uncompressedBufferPos as starting position

        uint16_t byteOffset;
        if (lowpanFragOffset == 0) {
            byteOffset = uncompressedBufferPos;
        } else {
            byteOffset = uncompressedBufferPos +
                    offsetToByteSize(lowpanFragOffset - uncompressedOffset);
        }

        // add frame to the buffer
        buffer->addFrame(frame, byteOffset);

        return true;
    } else {
        for (uint8_t i = 0; i < byteSizeToOffset(uncompressedSize); i++) {
            if ((occupied[bitmapArrayIndex] & bitmapBitMask) == 0) {
                // fragments are overlapping
                return false;
            }
            nextBitInBitMap(bitmapArrayIndex, bitmapBitMask);
        }
        return true;
    }
}


IPv6Datagram* DatagramReassembly::getDatagram() {
    // TODO we here cleanup the BufferInformation object, which means, that
    // the memory it is pointing to can potentially be reused, although we
    // still keep an pointer to precisely this memory in the datagram's
    // attached UDP packet --- we should really move the responsibility
    // of giving back the memory to the UDPPacket class
    if (isDone()) {
        // parse the uncompressed headers from the now complete datagram buffer
        uint16_t pos = uncompressedBufferPos;
        if (dcDatagram->uncompressedNext(buffer, pos)) {

            IPv6Datagram* ret = dcDatagram->decapsulateIPDatagram();
            delete dcDatagram;
            dcDatagram = NULL;
            return ret;
        }
        free();
    }
    return NULL;
}

} // namspace
