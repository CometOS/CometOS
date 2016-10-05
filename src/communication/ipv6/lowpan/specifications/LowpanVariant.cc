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

#include <LowpanVariant.h>

namespace cometos_v6 {

LowpanVariant::LowpanVariant(
    LowpanAdaptionLayerStats& stats,
    FragmentHandler& fragmentHandler,
    ManagedBuffer& buffer,
#ifdef OMNETPP
    omnetpp::cOutVector& BufferSizeVector,
    omnetpp::cOutVector& BufferNumElemVector,
#endif
    IPv6Context (&srcContexts)[16],
    IPv6Context (&dstContexts)[16],
    retransmissionList& ipRetransmissionList,
    LowpanAdaptionLayer* lowpan) :
            stats(stats),
            fragmentHandler(fragmentHandler),
            buffer(buffer),
#ifdef OMNETPP
            BufferSizeVector(BufferSizeVector),
            BufferNumElemVector(BufferNumElemVector),
#endif
            srcContexts(srcContexts),
            dstContexts(dstContexts),
            _ipRetransmissionList(ipRetransmissionList),
            _lowpan(lowpan) {
}

LowpanVariant::~LowpanVariant() {
}

void LowpanVariant::retrieveIpBaseHeader(cometos::Airframe& frame,
                          uint8_t lowpanDispatch,
                          const Ieee802154MacAddress& src,
                          const Ieee802154MacAddress& dst,
                          IPv6Datagram& dg)
{
    LOG_DEBUG("Retrieving IPv6 datagram ");
    IPHCDecompressor decompressor(lowpanDispatch,
                                          frame,
                                          src,
                                          dst,
                                          srcContexts,
                                          dstContexts);
    LOG_DEBUG("retrieved IPv6 datagram");
    const IPv6Datagram* parsedDg = decompressor.getIPDatagram();
    dg.dst = parsedDg->dst;
    dg.src = parsedDg->src;
    dg.setHopLimit(parsedDg->getHopLimit());
    dg.setTrafficClass(parsedDg->getTrafficClass());
    dg.setFlowLabel(parsedDg->getFlowLabel());
}

void LowpanVariant::extractUnfragmentedIP(cometos::Airframe& frame,
                                          uint8_t lowpanDispatch,
                                          const Ieee802154MacAddress& src,
                                          const Ieee802154MacAddress& dst,
                                          IPv6DatagramInformation* dgInfo)
{
    IPHCDecompressor dcip(lowpanDispatch, frame, src, dst, srcContexts,
                                   dstContexts);

    if (dcip.getIPDatagram() == NULL) {
        LAL_SCALAR_INC(dropped_Invalid);
        LOG_ERROR("Error in Datagram");
        return;
    }

    ManagedBuffer::MbRequestStatus status;
    BufferInformation* buf = buffer.getBuffer(frame.getLength(), status);

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer.getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer.getNumBuffers());
#endif

    if (buf == NULL) {
        if (status == ManagedBuffer::MbRequestStatus::FAIL_MEMORY) {
            LOG_ERROR("Buffer Full");
            LAL_SCALAR_INC(dropped_BufferFull);
        } else if (status == ManagedBuffer::MbRequestStatus::FAIL_HANDLERS) {
            LOG_ERROR("Buffer out of handlers");
            LAL_SCALAR_INC(dropped_BufferOutOfHandlers);
        } else {
            ASSERT(false);
        }
        return;
    }

    FirstFragBufInfo SBP = dcip.compressedNext(frame, buf);
    uint16_t bpos = SBP.numWrittenToBuf;
    if (frame.getLength() > 0) {
        for (uint8_t i = 0; frame.getLength() > 0; i++) {
            frame >> (*buf)[SBP.numWrittenToBuf++];
        }
    } else {
        LOG_DEBUG("Nothing left in frame");
    }

    if (dcip.uncompressedNext(buf, bpos)) {
        IPv6Datagram* datagram = dcip.decapsulateIPDatagram();
        // generate LlRxInfo object and attach to data indication
        LlRxInfo* tmpLlRxPtr = NULL;
        if (frame.has<cometos::MacRxInfo>()) {
            tmpLlRxPtr = new LlRxInfo(frame.get<cometos::MacRxInfo>());
        }
        dgInfo->setData(datagram, buf, tmpLlRxPtr);

    } else {
        LAL_SCALAR_INC(dropped_Invalid);
        LOG_ERROR("Error in Datagram");
        buf->free();
    }
}

void LowpanVariant::getSrcDstAddrFromMesh(cometos::Airframe& frame,
                                          const uint8_t head,
                                          Ieee802154MacAddress& src,
                                          Ieee802154MacAddress& dst) {
    removeHopsLftFieldFrmFrame(frame, head);
    removeSrcAddrFieldFrmFrame(frame, head, src);
    removeDstAddrFieldFrmFrame(frame, head, dst);
    LOG_DEBUG("Mesh Header, f:" << src.a4() << ", t:" << dst.a4());
}

void LowpanVariant::removeHopsLftFieldFrmFrame(cometos::Airframe& frame,
                                               uint8_t head) {
    uint8_t t;
    if ((head & 0xF) == 0xF) {
        frame >> t;
    }
}

void LowpanVariant::removeSrcAddrFieldFrmFrame(cometos::Airframe& frame,
                                               uint8_t head,
                                               Ieee802154MacAddress& src) {
    bool isSrcAddress16Bits = ((head & 0x20) == 0x20);
    if (isSrcAddress16Bits) {
        uint16_nbo a;
        frame >> a;
        src = Ieee802154MacAddress(a.getUint16_t());
    } else {
        uint16_nbo a, b, c, d;
        frame >> a >> b >> c >> d;
        src = Ieee802154MacAddress(a.getUint16_t(), b.getUint16_t(),
                                   c.getUint16_t(), d.getUint16_t());
    }
}

void LowpanVariant::removeDstAddrFieldFrmFrame(cometos::Airframe& frame,
                                               uint8_t head,
                                               Ieee802154MacAddress& dst) {
    bool isDstAddress16Bits = ((head & 0x10) == 0x10);
    if (isDstAddress16Bits) {
        uint16_nbo a;
        frame >> a;
        dst = Ieee802154MacAddress(a.getUint16_t());
    } else {
        uint16_nbo a, b, c, d;
        frame >> a >> b >> c >> d;
        dst = Ieee802154MacAddress(a.getUint16_t(), b.getUint16_t(),
                                   c.getUint16_t(), d.getUint16_t());
    }
}
IPHCDecompressor* LowpanVariant::getDecompressorFromFrame(
        cometos::Airframe& frame,
        const Ieee802154MacAddress& src,
        const Ieee802154MacAddress& dst,
        const IPv6Context* srcContexts,
        const IPv6Context* dstContexts)
{
    uint8_t ipHeaderCompressionIndicator;
    frame >> ipHeaderCompressionIndicator;
    IPHCDecompressor* decomp =
        new IPHCDecompressor(ipHeaderCompressionIndicator, frame, src,
                                      dst, srcContexts, dstContexts);

    bool isheaderDecompressionSuccessful =
        (decomp->getIPDatagram() != NULL);

    if (isheaderDecompressionSuccessful) {
        return decomp;
    } else {
        delete decomp;
        return NULL;
    }
}

void LowpanVariant::updateStatsOnReturnValue(bufStatus_t retVal) {
    switch (retVal) {
        case BS_OUT_OF_SPACE:
            LOG_WARN("Dropped BufFull");
            LAL_SCALAR_INC(dropped_BufferFull);
            break;
        case BS_OUT_OF_HANDLERS:
            LOG_WARN("Dropped Buffer out of Handlers");
            LAL_SCALAR_INC(dropped_BufferOutOfHandlers);
            break;
        case BS_OUT_OF_SPACE_ASSEMBLY:
            LOG_WARN("Dropped AssemlFull");
            LAL_SCALAR_INC(dropped_AssemblyBufferFull);
            break;
        case BS_OUT_OF_SPACE_TINY:
            LOG_WARN("Dropped TinyFull");
            LAL_SCALAR_INC(dropped_TinyBufferFull);
            break;
        case BS_NO_ROUTE:
            LOG_WARN("Dropped NoRoute");
            LAL_SCALAR_INC(dropped_NoRoute);
            break;
        case BS_NO_HOPS_LEFT:
            LOG_WARN("Dropped HopsLeft");
            LAL_SCALAR_INC(dropped_HopsLeft);
            break;
        case BS_QUEUE_FULL:
            LOG_WARN("Dropped QueueFull");
            LAL_SCALAR_INC(dropped_QueueFull);
            break;
        case BS_FRAGMENT_INVALID_ORDER:
            LOG_WARN("Dropped InvalidOrder");
            LAL_SCALAR_INC(dropped_InvalidOrder);
            break;
        case BS_FRAMGENT_NO_ENTRY:
            LOG_WARN("Dropped InvalidOrderMissing");
            LAL_SCALAR_INC(dropped_InvalidOrderMissing);
            break;
        case BS_OUT_OF_MESSAGES:
            LOG_WARN("Dropped OutOfMessages");
            LAL_SCALAR_INC(dropped_OutOfMessages);
            break;
        case BS_SUCCESS_PACKET:
            LOG_DEBUG("All Fragments of DF Packet arrives");
            LAL_SCALAR_INC(receivedPackets);
            break;
        default:
            break;
    }
}

bool LowpanVariant::isReassemblyComplete(DatagramReassembly* dGramInfo) {
    if (dGramInfo == NULL) return false;

    LOG_DEBUG("Went to Assembly Buffer cs: " << dGramInfo->getContiguousSize()
                                             << " " << (uintptr_t)dGramInfo);

    if (dGramInfo->isDone()) {
        LOG_DEBUG("Last Fragment");
        return true;
    } else {
        return false;
    }
}

} /* namespace cometos_v6 */

