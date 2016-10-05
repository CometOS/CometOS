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

#include "QueuePacket.h"
#include "LowpanAdaptionLayer.h"
#include "lowpan-macros.h"

namespace cometos_v6 {

QueuePacket::QueuePacket(IPv6Request* req, uint16_t tag,
        BufferInformation* buf, uint16_t posInBuffer):
        ipRequest(req),
        comprDatagramm(req),
        offset(0),
        size(req->data.datagram->getCompleteHeaderLength()
                + req->data.datagram->getUpperLayerPayloadLength()),
        tag(tag),
        buffer(buf),
        posInBuffer(posInBuffer)
{
    ASSERT(buffer->isUsed());
}

QueuePacket::~QueuePacket() {
    if (buffer != NULL && buffer->isUsed()) {
        buffer->free();
    }
    if (ipRequest != NULL) {
        ipRequest->response(new IPv6Response(ipRequest, IPv6Response::IPV6_RC_QUEUE_ABORT));
    }
}

QueueObject::response_t QueuePacket::response(bool success, const cometos::MacTxInfo & info) {
    txInfo.increaseNumRetries(info.numRetries);
    if (success) {
        txInfo.increaseNumTransmissions(1);
        if (buffer->getSize() == 0) {
            txInfo.setSuccess();
            buffer->free();
            buffer = NULL;
            IPv6Response * resp = new IPv6Response(ipRequest, IPv6Response::IPV6_RC_SUCCESS);
            resp->set(new LlTxInfo(txInfo));
            ipRequest->response(resp);
            ipRequest = NULL;
            return QueueObject::QUEUE_PACKET_FINISHED;
        }
    } else {
        buffer->free();
        buffer = NULL;
        IPv6Response * resp = new IPv6Response(ipRequest, IPv6Response::IPV6_RC_QUEUE_ABORT);
        resp->set(new LlTxInfo(txInfo));
        ipRequest->response(resp);
        ipRequest = NULL;
        return QueueObject::QUEUE_DELETE_OBJECT;
    }
    return QueueObject::QUEUE_KEEP;
}

void QueuePacket::createFrame(cometos::Airframe& frame,
                              uint8_t maxSize,
                              LowpanFragMetadata& fragMeta,
                              const IPv6Datagram* & dg) {
    ASSERT(buffer->isUsed());
    uint8_t offsetBeforeCompression = byteSizeToOffset(offset);


    uint8_t fragSize; //< actual uncompressed size of the data in this fragment

    bool fragmented = false;
    if (comprDatagramm.isDone()) {
        // if compression is done and we are called again here, we have
        // a fragmented datagram, of which we send one of the remaining
        // fragments now
        fragmented = true;

        if (posInBuffer > 0) {
            buffer->freeBegin(posInBuffer);
            posInBuffer = 0;
        }

        // truncate remaining fragment size after fragment header to multiples of 8 octets
        maxSize = (maxSize - LOWPAN_FRAG_HEADERX_LENGTH) & (~(LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT-1));
        fragSize = maxSize > buffer->getSize() ? buffer->getSize():maxSize;

        // use buffer part to copy data from buffer into frame; this also
        // frees the part of the buffer written to the frame
        bufferPart bp = buffer->getBufferPart(fragSize);
        frame << bp;
        LOG_DEBUG("Processed fragment with offset=" << (int) offset
                    << "|fragSize=" << (int) fragSize);
        offset += fragSize;
    } else {
        // write or continue to write datagram headers to frame
        fragSize = comprDatagramm.streamDatagramToFrame(frame, maxSize, buffer, posInBuffer);
        offset += fragSize;
        if (offset < size) {
            fragmented = true;
        }
        LOG_DEBUG("Processed first frame; fragmented=" << fragmented << "|nextOffset=" << (int) offset);
    }

    // set fragmentation meta data
    fragMeta.size = fragSize;
    fragMeta.dgSize = size;
    fragMeta.offset = offsetBeforeCompression;
    fragMeta.tag = tag.getUint16_t();

    dg = ipRequest->data.datagram;

    if (!comprDatagramm.isDone() || fragmented) {
        QueueObject::addFragmentHeader(frame, tag, size, offsetBeforeCompression, fragMeta.congestionFlag);
    }
}

const Ieee802154MacAddress& QueuePacket::getDstMAC() const {
    return ipRequest->data.dstMacAddress;
}

uint8_t QueuePacket::getPRCValue() const {
    return ((255 * offset) / size);
}
uint8_t QueuePacket::getSRCValue() const {
    return byteSizeToOffset(offset);
}
uint8_t QueuePacket::getHRCValue() const {
    return 0;
}

LocalDgId QueuePacket::getDgId(bool& valid) const {
    valid = ipRequest != NULL;
    if (valid) {
        return LocalDgId(ipRequest->data.srcMacAddress,
            ipRequest->data.dstMacAddress,
            this->tag.getUint16_t(),
            this->size);
    } else {
        return LocalDgId(0xffff, 0xffff, 0, 0);
    }
}

const IPv6Datagram* QueuePacket::getDg() const {
    return ipRequest->data.datagram;
}

bool QueuePacket::belongsTo(
        Ieee802154MacAddress const & src,
        Ieee802154MacAddress const & dst,
        uint16_t tag,
        uint16_t size) const
{
    return src == ipRequest->data.srcMacAddress
            && dst == ipRequest->data.dstMacAddress
            && tag == this->tag.getUint16_t()
            && size == this->size;
}

uint8_t QueuePacket::currOffset() const {
    return byteSizeToOffset(offset);
}

uint16_t QueuePacket::getCurrDgSize() const {
    return size;
}

}

