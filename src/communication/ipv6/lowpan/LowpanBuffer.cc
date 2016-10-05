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
 * @author Martin Ringwelski
 */

#include "LowpanBuffer.h"
#include "logging.h"

namespace cometos_v6 {

uint16_t BufferInformation::freeEnd(uint16_t EndSpace) {
    ASSERT(used);
    if (size <= EndSpace) {
        size = 0;
    } else {
        size -= EndSpace;
    }
    return size;
}

uint16_t BufferInformation::freeBegin(uint16_t BeginSpace) {
    ASSERT(used);
    if (size <= BeginSpace) {
        size = 0;
    } else {
        startPos += BeginSpace;
        size -= BeginSpace;
    }
    return size;
}

void BufferInformation::copyToBuffer(const uint8_t* data, uint16_t bSize, uint16_t pos) {
    ASSERT(used && size >= (bSize + pos));

    //        memcpy((startPos + pos), data, bSize);
    for (uint16_t i = 0; i < bSize; i++) {
        *(startPos + pos + i) = data[i];
    }
}

uint8_t BufferInformation::streamOut() {
    ASSERT(used && (size > 0));
    uint8_t ret = (*startPos);
    startPos++;
    size--;
    return ret;
}

cometos::pktSize_t BufferInformation::addFrame(cometos::Airframe& frame, uint16_t& pos) {
    LOG_DEBUG("used=" << used << "|size=" << size
               << "|len=" << (int) frame.getLength() << "|pos=" <<(int) pos);
    ASSERT(used && (size >= (frame.getLength() + pos)));
    cometos::pktSize_t ret = frame.getLength();
    while(frame.getLength() > 0) {
        frame >> *(startPos + pos);
        pos++;
    }
    return ret;
}

void BufferInformation::clean() {
    memset(startPos, 0, size);
}

void BufferInformation::populateFrame(cometos::Airframe& frame,
                                      uint16_t posInBuffer,
                                      uint16_t blockSize) {
    ASSERT(posInBuffer < size);
    if(blockSize == 0) return;
    if((posInBuffer + blockSize) > size) blockSize = (size - posInBuffer);
    uint8_t* basePtr = startPos + posInBuffer;
    while(--blockSize > 0){
        frame << *(basePtr + blockSize);
    }
    frame << *(basePtr);
}

//// ManagedBuffer ////////////////////////////////////////////////////////////
ManagedBuffer::ManagedBuffer()
{}


BufferInformation* ManagedBuffer::getBuffer(uint16_t size, MbRequestStatus& result) {
    uint8_t* buffer = getBufArray();
    uint8_t* start = buffer;
    uint8_t mEntries = getNumHandlers();
    uint16_t bSize = getBufferSize();
    BufferInformation* handlers = getHandlers();
    result = SUCCESS;
    uint16_t freeSize = bSize;
    for (int16_t j = 0; (j < mEntries) && (freeSize >= size); j++) {
        if ((handlers[j].used) && (handlers[j].size > 0) && (handlers[j].startPos >= start)) {
            uint16_t tmpFreeSize = handlers[j].startPos - start;
            if (freeSize > tmpFreeSize) {
                freeSize = tmpFreeSize;
                if (freeSize < size) {
                    start = handlers[j].startPos + handlers[j].size;
                    ASSERT(start > buffer);
                    freeSize = bSize - (start - buffer);
                    j = -1;
                }
            }
        }
    }

    if (freeSize >= size) {
        for (uint8_t i = 0; i < mEntries; i++) {
            if (!handlers[i].used) {
                handlers[i].used = true;
                handlers[i].size = size;
                handlers[i].startPos  = start;
                ASSERT(start >= buffer);
                ASSERT(size <= bSize);
                ASSERT(freeSize >= size);
                ASSERT((uint16_t)(start - buffer + size) <= bSize);
                return &(handlers[i]);
            }
        }
        result = FAIL_HANDLERS;
    } else {
        result = FAIL_MEMORY;
    }
    return NULL;
}

BufferInformation* ManagedBuffer::getBuffer(uint16_t size) {
    MbRequestStatus tmp;
    BufferInformation* buf = getBuffer(size, tmp);
    return buf;
}


BufferInformation* ManagedBuffer::getCorrespondingBuffer(const uint8_t* pointer) {
    BufferInformation* handlers = getHandlers();
    uint8_t mEntries = getNumHandlers();
    for (uint8_t i = 0; i < mEntries; i++) {
        if (handlers[i].used &&
                (handlers[i].startPos <= pointer) &&
                (&(handlers[i].startPos[handlers[i].size]) > pointer)) {
            return &(handlers[i]);
        }
    }
    return NULL;
}


void ManagedBuffer::clearAll() {
    BufferInformation* handlers = getHandlers();
    uint8_t mEntries = getNumHandlers();
    for (uint8_t i = 0; i < mEntries; i++) {
        handlers[i].used = false;
    }
}

#ifdef LOWPAN_ENABLE_BUFFERSTATS
/**
 * getUsedBufferSize()
 * returns the size of allocated buffer space. (for statistics)
 */
uint16_t ManagedBuffer::getUsedBufferSize() const {
    BufferInformation* handlers = getHandlers();
    uint16_t ret = 0;
    for (uint8_t i = 0; i < mEntries; i++) {
        if (handlers[i].used) {
            ret += handlers[i].size;
        }
    }
    return ret;
}


uint8_t ManagedBuffer::getNumBuffers() const {
    BufferInformation* handlers = getHandlers();
    uint16_t ret = 0;
    for (uint8_t i = 0; i < mEntries; i++) {
        if (handlers[i].used) {
            ret++;
        }
    }
    return ret;
}
#endif




DynLowpanBuffer::DynLowpanBuffer(uint16_t bufsize, uint8_t numHandlers) :
        buffer(NULL),
        handlers(NULL),
        bSize(bufsize),
        mEntries(numHandlers)
{
    buffer = new uint8_t[bufsize];
    handlers = new BufferInformation[numHandlers];
    clearAll();	
}

DynLowpanBuffer::~DynLowpanBuffer() {
    delete[] buffer;
    buffer = NULL;

    delete[] handlers;
    handlers = NULL;
}

} // namespace


namespace cometos {

void serialize(ByteVector & buf, const cometos_v6::bufferPart & val) {
    uint8_t size = val.sizeStart;
    while (size > 0) {
        size--;
        serialize(buf, (*val.buf)[size]);
    }
    // TODO is it wise to have this side-effect here?
    val.buf->freeBegin(val.sizeStart);
}

void unserialize(ByteVector & buf, cometos_v6::bufferPart & val) {
    uint16_t pos = val.sizeStart;
    while (buf.getSize() > 0) {
        unserialize(buf, (*val.buf)[pos++]);
    }
}

} /* namespace cometos */

