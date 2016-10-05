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

#ifndef LOWPANBUFFER_H_
#define LOWPANBUFFER_H_

//#include <string>
#include "cometos.h"
#include "Airframe.h"
#include "palLed.h"
#include "OutputStream.h"

#ifdef OMNETPP
#ifndef LOWPAN_ENABLE_BIGBUFFER
#define LOWPAN_ENABLE_BIGBUFFER
#endif
#endif

namespace cometos_v6 {

class BufferInformation;

class bufferPart {
public:
    BufferInformation*  buf;
    uint8_t             sizeStart;

    bufferPart(BufferInformation* buf, uint8_t size): buf(buf), sizeStart(size) {}
};

class BufferInformation {
public:
   friend class ManagedBuffer;

    BufferInformation():
        startPos(0),
        size(0),
        used(false) {}

    virtual ~BufferInformation() {}

    inline bool isUsed() const {
        return used;
    }

    virtual void free() {
        ASSERT(used);
        used = false;
    }

    virtual uint16_t freeEnd(uint16_t EndSpace);
    virtual uint16_t freeBegin(uint16_t BeginSpace);

    uint16_t getSize() const {
        ASSERT(used);
        if (used) return size;
        return 0;
    }

    /**
     * getContent
     * Returns a constant pointer to beginning of the buffer.
     * Only for reading purposes.
     */
    const uint8_t* getContent() const {
        ASSERT(used);
        if (used) return startPos;
        return NULL;
    }

    /**
     * copyToBuffer
     * Wrapper for memcpy
     */
    void copyToBuffer(const uint8_t* data, uint16_t bSize, uint16_t pos = 0);

    /**
     * operator[]
     * safe way to access all bytes of the buffer (read and write)
     */
    uint8_t& operator[](uint16_t field) {
        ASSERT(used && field < size);
        return startPos[field];
    }

    /**
     * streamOut()
     * returns the first byte from the buffer and decreases the size.
     */
    uint8_t streamOut();

    /**
     * Returns a bufferPart object which can be used to stream it into an
     * Airframe.
     * see: void serialize(ByteVector & buf, const cometos_v6::bufferPart & val)
     */
    bufferPart getBufferPart(uint8_t pSize) {
        if (pSize > size) pSize = size;
        return bufferPart(this, pSize);
    }

	 /**
     * Stream an Airframe to the buffer at the given position.
     */
    cometos::pktSize_t addFrame(cometos::Airframe& frame, uint16_t & pos);

    void clean();

    /*
     * purpose: fill the content of the buffer into frame. A block of content
     * of size blockSize is put into the frame. The data at (posInBuffer
     * + blockSize - 1) is copied first into the frame followed by data all
     * the way to (posInBuffer), Basically the range
     * (posInBuffer + BlockSize, posInBuffer]. If posInBuffer + blockSize exceeds
     * the size of buffer, only content from the end of the buffer to posInBuffer is copied
     * into the frame. Requires that posInBuffer < size of the buffer.
     */
    void populateFrame(cometos::Airframe& frame,
                       uint16_t posInBuffer,
                       uint16_t blockSize);

private:
    uint8_t*    startPos;   ///< Pointer to the beginning of the Buffer
    uint16_t    size;       ///< Size of the Buffer
    bool        used;       ///< Used flag
};

class ManagedBuffer {
public:
#if defined SWIG || defined BOARD_python
    enum MbRequestStatus {
#else
    enum MbRequestStatus : uint8_t {
#endif
        SUCCESS,
        FAIL_MEMORY,
        FAIL_HANDLERS
    };

    typedef uint16_t bufSize_t;

    ManagedBuffer();
    virtual ~ManagedBuffer() {}

    BufferInformation* getBuffer(bufSize_t size, MbRequestStatus& result);
    BufferInformation* getBuffer(bufSize_t size);


    /**
     * getCorrespondingBuffer
     * Checks if the pointer points to a space in the buffer and returns the
     * corresponding BufferInformation object.
     * @param pointer   Pointer to check
     */
    BufferInformation* getCorrespondingBuffer(const uint8_t* pointer);

    /**
     * clearAll
     * Sets all buffers free.
     */
    void clearAll();
#ifdef LOWPANBUFFER_ENABLESTATS

    /**
     * getUsedBufferSize()
     * returns the size of allocated buffer space. (for statistics)
     */
    bufSize_t getUsedBufferSize() const;

    /**
     * getNumBuffers()
     * returns the number of allocated buffers. (for statistics)
     */
    uint8_t getNumBuffers() const;
#endif

private:
    virtual uint8_t* getBufArray() = 0;
    virtual BufferInformation* getHandlers() = 0;
    virtual bufSize_t getBufferSize() const = 0;
    virtual uint8_t getNumHandlers() const = 0;
};



/**
 * Variant of the managed buffer using dynamic memory allocation.
 */
class DynLowpanBuffer : public ManagedBuffer {
public:
    DynLowpanBuffer(bufSize_t bufsize, uint8_t numHandlers);

    ~DynLowpanBuffer();

private:
    virtual uint8_t* getBufArray() {
        return buffer;
    }
    virtual BufferInformation* getHandlers() {
        return handlers;
    }
    virtual bufSize_t getBufferSize() const {
        return bSize;
    }
    virtual uint8_t getNumHandlers() const {
        return mEntries;
    }

    uint8_t* buffer;
    BufferInformation* handlers;
    bufSize_t bSize;
    uint8_t mEntries;
};






template <ManagedBuffer::bufSize_t BSize, uint8_t MEntries>
class LowpanBuffer: public ManagedBuffer {
public:
    LowpanBuffer()
#ifdef LOWPAN_ENABLE_BIGBUFFER
        : bSize(BSize), mEntries(MEntries)
#endif
    {
        clearAll();
    }
    ~LowpanBuffer() {}

private:
    virtual uint8_t* getBufArray() {
        return buffer;
    }

    virtual BufferInformation* getHandlers() {
        return handlers;
    }

    virtual uint8_t getNumHandlers() const {
#ifdef LOWPAN_ENABLE_BIGBUFFER
        return mEntries;
#else
        return MEntries;
#endif
    }

    virtual bufSize_t getBufferSize() const {
#ifdef LOWPAN_ENABLE_BIGBUFFER
        return bSize;
#else
        return BSize;
#endif
    }

#ifdef LOWPAN_ENABLE_BIGBUFFER
    void setMEntries(uint8_t mEntries) {
        this->mEntries = mEntries;
    }

    void setBSize(bufSize_t bSize) {
        this->bSize = bSize;
    }

#endif

protected:
    uint8_t buffer[BSize];

    BufferInformation   handlers[MEntries];

#ifdef LOWPAN_ENABLE_BIGBUFFER
    bufSize_t    bSize;
    uint8_t     mEntries;
#endif

};

} /* namespace cometos_v6 */

namespace cometos {

void serialize(ByteVector & buf, const cometos_v6::bufferPart & val);
void unserialize(ByteVector & buf, cometos_v6::bufferPart & val);

} /* namespace cometos */

#endif /* LOWPANBUFFER_H_ */
