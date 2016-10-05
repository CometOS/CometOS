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
 * @author Martin Ringwelski, Andreas Weigel
 */


#include "AssemblyBuffer.h"
#include "lowpan-macros.h"

namespace cometos_v6 {

AssemblyBufferBase::AssemblyBufferBase(DatagramReassembly* handlers,
                       uint8_t numHandlers,
                       ManagedBuffer* lowpanBuf) :
                           handlers(handlers),
                           mEntries(numHandlers),
                           lBuffer(lowpanBuf)
{}

AssemblyBufferBase::~AssemblyBufferBase() {
}

/**
 *
 */
DatagramReassembly* AssemblyBufferBase::addFragment(
        const Ieee802154MacAddress& srcMAC,
        uint16_t tag,
        uint16_t size,
        uint8_t offset,
        cometos::Airframe& frame,
        bufStatus_t & status)
{
    status = BS_SUCCESS;
    DatagramReassembly* handler = findCorrespondingId(srcMAC, tag, size);
#ifdef LOWPAN_ENABLE_MESH
    if (NULL == handler) {
        handler = getBuffer(srcMAC, tag, size);
    }
#endif
    cometos::pktSize_t len = frame.getLength();
    if (NULL != handler) {
        handler->addFragment(offset, frame, len);
        LOG_DEBUG("Added fragment: bitmap=" << cometos::hex << handler->getAckBitmap()
                    << "|done=" << handler->isDone());
    } else {
        LOG_DEBUG("Found no existing handler");
        status = BS_FRAMGENT_NO_ENTRY;
    }
    return handler;
}

DatagramReassembly* AssemblyBufferBase::addFirstFragment(
         const Ieee802154MacAddress& srcMAC,
         uint16_t tag,
         uint16_t size,
         cometos::Airframe& frame,
         IPHCDecompressor* & dcDatagram,
         bufStatus_t & status)
 {
    status = BS_SUCCESS;
    DatagramReassembly* handler = findCorrespondingId(srcMAC, tag, size);
    if (NULL == handler) {
        handler = getBuffer(srcMAC, tag, size, status);
    }

    // we should have a handler now, otherwise, there has been some error
    if (NULL != handler) {
        if (handler->getDcDatagram() == NULL) {
            FirstFragBufInfo SBP = dcDatagram->compressedNext(frame, handler->getBuffer());

            // attach decompressor to reassembly process and set additional
            // information about index of first uncompressed header and
            // the current write index within the buffer; those two are
            // identical here, because decompressor stops writing at first
            // uncompressed header
            handler->setDecompressor(dcDatagram);
            handler->setDecompressionInfo(SBP.numWrittenToBuf, byteSizeToOffset(SBP.uncompressedSize));


            // the total uncompressed size of the datagram is uncompressed
            // size of the headers + remaining uncompressed bytes in the frame
            if (!handler->addFragment(0, frame, SBP.uncompressedSize + frame.getLength())) {
                return NULL;
            }
        } else {
            // first fragment already received; no need to decompress again
            delete dcDatagram;
            dcDatagram = NULL;
        }
    } else {
        delete dcDatagram;
        dcDatagram = NULL;
    }
    return handler;
 }

DatagramReassembly* AssemblyBufferBase::addFirstLFFRFragment(
         const Ieee802154MacAddress& srcMAC,
         uint16_t tag,
         uint16_t size,
         cometos::Airframe& frame,
         IPHCDecompressor* & dcDatagram,
         bufStatus_t & status, bool enableImplicitAck){
    DatagramReassembly* fragmentInfo = addFirstFragment(srcMAC,
            tag, size, frame, dcDatagram, status);
    if(fragmentInfo != NULL){
        uint8_t seqNum = 0;
        fragmentInfo->storeSequenceNumberInAckBitmap(seqNum);
    }

    return fragmentInfo;
}

DatagramReassembly* AssemblyBufferBase::addSubsequentLFFRFragment(
        const Ieee802154MacAddress& srcMAC,
        uint16_t tag,
        uint16_t size,
        uint8_t offset,
        uint8_t seqNum,
        cometos::Airframe& frame,
        bufStatus_t & status,
        bool enableImplicitAck){
    DatagramReassembly* dr = addFragment(srcMAC,
            tag, size, offset, frame, status);
    if(dr != NULL){
        dr->storeSequenceNumberInAckBitmap(seqNum);
    }
    return dr;
}

bool AssemblyBufferBase::handleRFRAG_ACKMessage(uint16_t& receivedTag,
                            const Ieee802154MacAddress& srcMAC,
                            uint32_t& receivedAckBitmap,
                            bufStatus_t& status,
                            bool enableECN) {
    ASSERT(false);
    return false;
}

uint8_t AssemblyBufferBase::tick() {
    uint8_t timeouts = 0;
    for (uint8_t i = 0; i < mEntries; i++) {
        if (handlers[i].getStatus() != ASSEMBLYBUFFER_STATUS_FREE
                && handlers[i].getStatus() != ASSEMBLYBUFFER_STATUS_DONE) {

            // starting from ASSEMBLYBUFFER_STATUS_RECEIVED, each timeout
            // period decreases the ticks counter; if it reaches
            // ASSEMBLYBUFFER_STATUS_FREE, the datagram is considered timed
            // out
            handlers[i].tick();
            if (handlers[i].getStatus() == ASSEMBLYBUFFER_STATUS_FREE) {
                timeouts++;
                handlers[i].free();
            }
        }
    }
    return timeouts;
}

void AssemblyBufferBase::setMEntries(uint8_t entries) {
    mEntries = entries;
}

DatagramReassembly* AssemblyBufferBase::findCorrespondingId(
        const Ieee802154MacAddress& srcMAC,
        uint16_t tag,
        uint16_t size) {
    for (uint8_t id = 0; id < mEntries; id++) {
        if ((handlers[id].getStatus() != ASSEMBLYBUFFER_STATUS_FREE) &&
                handlers[id].belongsToDatagram(srcMAC, tag, size))
        {
            return &(handlers[id]);
        }
    }
    return NULL;
}

DatagramReassembly* AssemblyBufferBase::getBuffer(
        const Ieee802154MacAddress& srcMAC,
        uint16_t tag,
        uint16_t dgSize,
        bufStatus_t & status) {
    uint8_t id = 0;
    for (; (id < mEntries) &&
           (handlers[id].getStatus() != ASSEMBLYBUFFER_STATUS_FREE);
        id++);
    if (id < mEntries) {
        ManagedBuffer::MbRequestStatus mbStatus;

        // we do not need storage for the IPv6 header itself, which is
        // always carried in the accompanying datagram
        uint16_t reqBufSize = dgSize - IPv6Datagram::IPV6_HEADER_SIZE;
        BufferInformation* buffer = lBuffer->getBuffer(reqBufSize, mbStatus);
        LOG_DEBUG("Acquired buffer of size " << reqBufSize);
        if (NULL != buffer) {
            handlers[id].initialize(srcMAC, buffer, tag, dgSize);
            return &(handlers[id]);
        }

        if (mbStatus == ManagedBuffer::FAIL_HANDLERS) {
            status = BS_OUT_OF_HANDLERS;
        } else {
            status =  BS_OUT_OF_SPACE;
        }
    } else {
        status =  BS_OUT_OF_SPACE_ASSEMBLY;
    }
    return NULL;
}


DynAssemblyBuffer::DynAssemblyBuffer(ManagedBuffer* buffer, uint8_t MEntries) :
        AssemblyBufferBase(new DatagramReassembly[MEntries], MEntries, buffer)
{
}

DynAssemblyBuffer::~DynAssemblyBuffer() {
    DatagramReassembly*& pHandlers = this->getHandlers();
    delete[] pHandlers;
    pHandlers = NULL;
}



}  // namespace cometos_v6
