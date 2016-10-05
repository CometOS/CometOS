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

#include <LFFRFrame.h>

namespace cometos_v6 {

LFFRFrame::LFFRFrame(PacketInformation* directPacket,
                     BufferInformation* buffer,
                     uint8_t uncompressedPos,
                     uint8_t dataGrmOffset,
                     uint8_t sequenceNumber,
                     bool enableImplicitAck,
                     uint8_t typeOfUncontainedNextHeader)
    : directPacket(directPacket),
      buffer(buffer),
      _dataGrmOffset(dataGrmOffset),
      _sequenceNumber(sequenceNumber),
      _enableImplicitAck(enableImplicitAck),
      _typeOfUncontainedNextHeader(typeOfUncontainedNextHeader)
//      ,_uncompressedPos(uncompressedPos)
{
    ASSERT(directPacket->isFree() == false);
}

LFFRFrame::~LFFRFrame() {
    if (buffer != NULL) {
        buffer->free();
    }
}

const Ieee802154MacAddress& LFFRFrame::getDstMAC() const {
    return directPacket->getDstMAC();
}

uint8_t LFFRFrame::getPRCValue() const {
    ASSERT(false);
    return 0;
}

uint8_t LFFRFrame::getSRCValue() const {
    ASSERT(false);
    return 0;
}

QueueObject::response_t LFFRFrame::response(bool success,
                                            const cometos::MacTxInfo& info) {

    QueueObject::response_t ret = QueueObject::QUEUE_DELETE_OBJECT;
    if (directPacket != NULL) {
        directPacket->updateTxInfo(success, info);
    }
    // if the first fragment has been lost then there is no
    // point forwarding the rest of the fragments onward
    if (_sequenceNumber == 0 && success == false) {
        LOG_DEBUG("Forward of First Fragment with Rx tag: "
                  << directPacket->getTag()
                  << " failed , Dropping direct Buffer entry ");
        directPacket->setFree(success);
        // remove the rest of frames belonging to this packet from the
        // queue -> these will not be of use any way at the next node
        // as there would be no lookup. do not add to traffic
        // by forwarding these
        ret = QueueObject::QUEUE_DELETE_SIMILAR;
    }
    return ret;
}

void LFFRFrame::createFrame(cometos::Airframe& frame,
                            uint8_t maxSize,
                            LowpanFragMetadata& fragHead,
                            const IPv6Datagram* & dg) {
    ASSERT(directPacket->isFree() == false);
    cometos::pktSize_t fragSize; //< uncompressed fragment size
    if (_sequenceNumber == 0) {
        IPHCCompressor compressor(directPacket->getDatagram(),
                                                 directPacket->getSrcMAC(),
                                                 directPacket->getDstMAC(),
                                                 _typeOfUncontainedNextHeader);
        uint16_t posInBuffer = 0;
        fragSize = compressor.streamDatagramToFrame(frame, maxSize, buffer, posInBuffer);
    } else {
        bufferPart bp = buffer->getBufferPart(maxSize);
        frame << bp;  // consumes buffer
        fragSize = frame.getLength();
    }

    dg = directPacket->getDatagram();
    fragHead.dgSize = directPacket->getSize();
    fragHead.offset = _dataGrmOffset;
    fragHead.size = fragSize;
    fragHead.tag = directPacket->getNewTag();

    addLFFRHeader(frame, directPacket->getSize(), _sequenceNumber,
                               directPacket->getNewTag(), _dataGrmOffset,
                               _enableImplicitAck);
}

uint8_t LFFRFrame::getHRCValue() const {
    ASSERT(false);
    return 0;
}

bool LFFRFrame::canBeDeleted() const {
    if (directPacket->isFree()) {
        return true;
    }
    return false;
}

LocalDgId LFFRFrame::getDgId(bool& valid) const {
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

const IPv6Datagram* LFFRFrame::getDg() const {
    return directPacket->getDatagram();
}

bool LFFRFrame::belongsTo(
        Ieee802154MacAddress const & src,
        Ieee802154MacAddress const & dst,
        uint16_t tag,
        uint16_t size) const {
    return src == directPacket->getSrcMAC()
            && dst == directPacket->getDstMAC()
            && tag == directPacket->getTag()
            && size == directPacket->getSize();
}

uint8_t LFFRFrame::currOffset() const {
    return _dataGrmOffset;
}

uint16_t LFFRFrame::getCurrDgSize() const {
    return directPacket->getSize();
}

} /* namespace cometos_v6 */
