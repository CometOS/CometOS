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

#include "Rfc4944Specification.h"

namespace cometos_v6 {

Rfc4944Implementation::Rfc4944Implementation(
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
            LowpanVariant(stats,
                          fragmentHandler,
                          buffer,
#ifdef OMNETPP
                          BufferSizeVector,
                          BufferNumElemVector,
#endif
                          srcContexts,
                          dstContexts,
                          ipRetransmissionList,
                          lowpan)
{
}

Rfc4944Implementation::~Rfc4944Implementation() {
}

void Rfc4944Implementation::parseframe(cometos::Airframe& frame,
                                     uint8_t lowpanDispatch1,
                                     const Ieee802154MacAddress& src,
                                     const Ieee802154MacAddress& dst,
                                     IPv6DatagramInformation* dgInfo) {
    ASSERT(dgInfo != NULL);

    LowpanFragMetadata fragMeta;

    if (isFragmentationHeader(lowpanDispatch1)) {
        parseFragmentationHeader(frame, lowpanDispatch1, fragMeta);
        handleFragmentData(frame, fragMeta, src, dst, dgInfo);
    } else {
        LOG_DEBUG("No Fgmt");
        extractUnfragmentedIP(frame, lowpanDispatch1, src, dst, dgInfo);
    }
}

void Rfc4944Implementation::parseLowpanAndIpHeader(cometos::Airframe& frame,
                                                   uint8_t lowpanDispatch,
                                                   const Ieee802154MacAddress& src,
                                                   const Ieee802154MacAddress& dst,
                                                   LowpanFragMetadata& fragMeta,
                                                   IPv6Datagram& dg) {
    // TODO there is some not-so-beautiful code redundancy here. we would need
    // to break up the long spaghetti-sequence from parsing the header to
    // adding the content of the frame to the actual buffer -- however, I
    // currently do not have that one elegant idea :-/
    bool retrieveIpHeader = false;

    if (isFragmentationHeader(lowpanDispatch)) {
        parseFragmentationHeader(frame, lowpanDispatch, fragMeta);
        if (!fragMeta.isFirstOfDatagram()) {
            retrieveIpHeader = false;
        } else {
            retrieveIpHeader = true;
        }
        fragMeta.fragmented = true;
        fragMeta.congestionFlag = isBpCongestionFlagSet(lowpanDispatch);

        LOG_DEBUG("Getting next lowpan dispatch header; fl=" << (int) frame.getLength());
        // get dispatch byte from next lowpan header
        frame >> lowpanDispatch;
    } else {
        // if datagram is not fragmented, there should be an IP header in any
        // case, set flag for retrieval
        retrieveIpHeader = true;
        fragMeta.fragmented = false;
    }


    if (retrieveIpHeader) {
        retrieveIpBaseHeader(frame, lowpanDispatch, src, dst, dg);
    }

    if (fragMeta.fragmented == false) {
        ASSERT(retrieveIpHeader);
        fragMeta.size = 0;
        fragMeta.dgSize = 0;
        fragMeta.isFirst = true;
        fragMeta.offset = 0;
    }
}

void Rfc4944Implementation::parseFragmentationHeader(cometos::Airframe& frame,
                                                     uint8_t fragDispatch,
                                                     LowpanFragMetadata& fragMeta)
{
    if (!isValidFragmentFrame(frame, fragDispatch)) {
        return;
    }

    uint16_nbo dGramSize;
    uint16_nbo dGramTag;
    uint8_t dGramOffset = 0;

    readCommonFragmentationHeader(frame, fragDispatch, dGramSize, dGramTag);
    if (isFragmentationHeaderX(fragDispatch)) {
        frame >> dGramOffset;
    }
    LOG_DEBUG("parsed: t=" << dGramTag.getUint16_t()
                           << "|dgs=" << dGramSize.getUint16_t()
                           << "|offset=" << (uint16_t) dGramOffset);
    fragMeta.size = frame.getLength();
    fragMeta.isFirst = isFragmentationHeader1(fragDispatch);

    fragMeta.dgSize = dGramSize.getUint16_t();
    fragMeta.offset = dGramOffset;
    fragMeta.tag = dGramTag.getUint16_t();
}

void Rfc4944Implementation::handleFragmentData(cometos::Airframe& frame,
                                             const LowpanFragMetadata& fragMeta,
                                             const Ieee802154MacAddress& src,
                                             const Ieee802154MacAddress& dst,
                                             IPv6DatagramInformation* dgInfo) {

    DatagramReassembly* dgReassemblyInfo = NULL;

    if (!fragMeta.isFirstOfDatagram()){
        dgReassemblyInfo = continueReassemblyOfDgram(
            src, fragMeta, frame);

    } else {
        LOG_DEBUG("FragmentHeader1 Found");
        IPHCDecompressor* decompressor =
            getDecompressorFromFrame(frame, src, dst, srcContexts,
                                                dstContexts);

        if (decompressor != NULL) {
            LOG_DEBUG("Datagram from "
                      << decompressor->getIPDatagram()->src.str()
                      << " to "
                      << decompressor->getIPDatagram()->dst.str());

            dgReassemblyInfo = initiateReassemblyOfDgram(
                src, fragMeta.tag, fragMeta.dgSize, frame,
                decompressor);
        } else {
            LAL_SCALAR_INC(dropped_Invalid);
            LOG_WARN("No Valid IP Datagram");
        }
    }

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer.getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer.getNumBuffers());
#endif

    if (isReassemblyComplete(dgReassemblyInfo)) {
        IPv6Datagram* reassembledDg = dgReassemblyInfo->getDatagram();
        bool isReassembledDgValid = (reassembledDg != NULL);
        if (isReassembledDgValid) {
            dgInfo->setData(reassembledDg,
                                 dgReassemblyInfo->decapsulateBuffer(),
                                 dgReassemblyInfo->getLlRxInfo().getCopy());
        } else {
            LAL_SCALAR_INC(dropped_Invalid);
            LOG_ERROR("Datagram Missing");
        }
        dgReassemblyInfo->free();
    }
}

bool Rfc4944Implementation::isValidFragmentFrame(const cometos::Airframe& frame,
                                               uint8_t head) {
    // return and discard frame if size is invalid
    uint8_t lsbOfDgrmSize;
    uint16_nbo dGramTag;
    uint8_t dGramOffset;
    uint8_t ipHeaderCompressionIndicator;
    uint8_t expectedSizeofFragmentHeader;

    if (isFragmentationHeaderX(head))  // i.e is subsequent header
        expectedSizeofFragmentHeader =
            sizeof(lsbOfDgrmSize) + sizeof(dGramTag) + sizeof(dGramOffset);
    else
        expectedSizeofFragmentHeader = sizeof(lsbOfDgrmSize) +
                                       sizeof(dGramTag) +
                                       sizeof(ipHeaderCompressionIndicator);

    if (frame.getLength() < expectedSizeofFragmentHeader)
        return false;
    else
        return true;
}

DatagramReassembly* Rfc4944Implementation::continueReassemblyOfDgram(
        const Ieee802154MacAddress& srcMAC,
        const LowpanFragMetadata& fragMeta,
        cometos::Airframe& frame)
{
    LOG_DEBUG("FragmentHeaderX Found|t=" << (uint16_t) fragMeta.tag
                                << "|s=" << (uint16_t) fragMeta.size
                                << "|dgs=" << fragMeta.dgSize
                                << "|o=" << (uint16_t) fragMeta.offset);
    bufStatus_t retVal;
    DatagramReassembly* fi = fragmentHandler.addFragment(
        srcMAC, fragMeta.tag, fragMeta.dgSize, fragMeta.offset, frame, retVal);
    updateStatsOnReturnValue(retVal);
    return fi;
}


DatagramReassembly* Rfc4944Implementation::initiateReassemblyOfDgram(
        const Ieee802154MacAddress& src,
        uint16_t dGramTag,
        uint16_t dGramSize,
        cometos::Airframe& frame,
        IPHCDecompressor* decompressor)
{
    bufStatus_t retVal = BS_SUCCESS;
    DatagramReassembly* fi = fragmentHandler.addFirstFragment(
                                                src,
                                                dGramTag,
                                                dGramSize,
                                                frame,
                                                decompressor,
                                                retVal);
    updateStatsOnReturnValue(retVal);
    return fi;
}


void Rfc4944Implementation::readCommonFragmentationHeader(
        cometos::Airframe& frame,
        uint8_t head,
        uint16_nbo& dGramSize,
        uint16_nbo& dGramTag)
{
    uint8_t msbOfDgrmSize = (head & LOWPAN_FRAG_HEADER_MSB_SIZE_MASK);
    uint8_t lsbOfDgrmSize;
    frame >> lsbOfDgrmSize;
    dGramSize = (((uint16_t)(msbOfDgrmSize) << 8) | (lsbOfDgrmSize));
    frame >> dGramTag;
}


QueueObject* Rfc4944Implementation::processBufferSpaceAndReturnQueueObject(
        IPv6Request* req,
        uint16_t tag,
        BufferInformation* buf,
        uint16_t posInBuffer)
{
    LOG_INFO("Creating new QueuePacket; posInBuffer=" << posInBuffer);
    QueueObject* tmp = new QueuePacket(req, tag, buf, posInBuffer);
    return tmp;
}

} /* namespace cometos_v6 */

