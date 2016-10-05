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

#include "QueueFrame.h"

namespace cometos_v6 {

QueueFrame::QueueFrame(PacketInformation* directPacket,
        BufferInformation* buffer,
        uint8_t uncompressedPos,
        uint8_t offset,
        bool last,
        uint8_t typeOfUncontainedNextHeader):
            directPacket(directPacket),
            offset(offset),
            buffer(buffer),
            state(false, last),
            typeOfUncontainedNextHeader(typeOfUncontainedNextHeader),
            uncompressedPos(uncompressedPos)
{
    ASSERT(directPacket->isFree() == false);
}

QueueFrame::~QueueFrame() {
    if (buffer != NULL) {
        buffer->free();
    }
}

QueueObject::response_t QueueFrame::response(bool success, const cometos::MacTxInfo & info) {
    QueueObject::response_t ret = QueueObject::QUEUE_DELETE_OBJECT;
    if (directPacket != NULL) {
        directPacket->updateTxInfo(success, info);
    }

    if (!success) {
        ret = QueueObject::QUEUE_DELETE_SIMILAR;
    } else if (success && state.last) {
        ret = QueueObject::QUEUE_PACKET_FINISHED;
    }

    if ((!success) || state.last) {
        LOG_DEBUG("Release PI@" << (uintptr_t) directPacket);
        directPacket->setFree(success);
        directPacket = NULL;
    }

    return ret;
}

void QueueFrame::createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragMeta,
                             const IPv6Datagram* & dg) {
    ASSERT(directPacket->isFree() == false);

    uint8_t fragSize;
    if (offset > 0) {
        // expect content to live at the beginning of passed buffer
        // in this case, all content will be uncompressed
        LOG_DEBUG("Getting frame from buffer of " << (int) buffer->getSize() << "bytes") ;
        bufferPart bp = buffer->getBufferPart(maxSize);
        LOG_DEBUG("@" << this << "; Stream " << (int) bp.sizeStart << " bytes into frame; maxSize=" << (int) maxSize);
        frame << bp;
        buffer->freeBegin(bp.sizeStart);// redundant
        fragSize = frame.getLength();
    } else {
        Ieee802154MacAddress tmpMAC;
        IPHCCompressor comprDatagramm(directPacket->getDatagram(),
                        tmpMAC,
                        directPacket->getDstMAC(),
                        typeOfUncontainedNextHeader);
        fragSize = comprDatagramm.streamDatagramToFrame(frame, maxSize, buffer, uncompressedPos); //, true);
    }

    dg = directPacket->getDatagram();
    fragMeta.dgSize = directPacket->getSize();
    fragMeta.size = fragSize;
    fragMeta.tag = directPacket->getNewTag();
    fragMeta.offset = offset;
#ifdef ENABLE_LOGGING
    cometos::pktSize_t oldLen = frame.getLength();
#endif
    uint16_nbo tag(directPacket->getNewTag());
    QueueObject::addFragmentHeader(frame, tag, directPacket->getSize(), offset, fragMeta.congestionFlag);
    LOG_DEBUG("Add fragment header (" << (int) frame.getLength() - oldLen << " bytes) to frame; total=" << (int) frame.getLength());
    state.inTransmission = true;
}

const Ieee802154MacAddress& QueueFrame::getDstMAC() const {
    return directPacket->getDstMAC();
}

uint8_t QueueFrame::getPRCValue() const {
    return (offset / (directPacket->getSize() >> 3));
}

uint8_t QueueFrame::getSRCValue() const {
    return offset;
}

uint8_t QueueFrame::getHRCValue() const {
    return 0;
}

bool QueueFrame::canBeDeleted() const {
    if (!state.inTransmission && directPacket->isFree()) {
        return true;
    }
    return false;
}

LocalDgId QueueFrame::getDgId(bool& valid) const {
    // TODO to check for internals of another class is bad. PacketInformation
    // should should handle problems with having no request available by itself
    valid = directPacket->getRequest() != NULL;
    if (valid) {
        return LocalDgId(directPacket->getSrcMAC(),
            directPacket->getDstMAC(),
            directPacket->getTag(),
            directPacket->getSize());
    } else {
        return LocalDgId(0xffff, 0xffff, 0, 0);
    }
}

const IPv6Datagram* QueueFrame::getDg() const {
    return directPacket->getDatagram();
}

bool QueueFrame::belongsTo(Ieee802154MacAddress const & src,
        Ieee802154MacAddress const & dst,
        uint16_t tag,
        uint16_t size) const {
    return src == directPacket->getSrcMAC()
            && dst == directPacket->getDstMAC()
            && tag == directPacket->getTag()
            && size == directPacket->getSize();
}

uint8_t QueueFrame::currOffset() const {
    return offset;
}

uint16_t QueueFrame::getCurrDgSize() const {
    return directPacket->getSize();
}

}
