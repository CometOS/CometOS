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

#include "LFFRSpecification.h"
#include "LowpanAdaptionLayer.h"

namespace cometos_v6 {

LFFRImplementation::LFFRImplementation(
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
                          lowpan),
      RFRAG(0xE8),
      RFRAG_AR(0xE9),
      RFRAG_ACK(0xEA),
      RFRAG_AEC(0xEB) {
    // TODO Auto-generated constructor stub
}

LFFRImplementation::~LFFRImplementation() {
    // TODO Auto-generated destructor stub
}

void LFFRImplementation::parseFragmentationHeader(cometos::Airframe& frame,
                                                  uint8_t fragDispatch,
                                                  LowpanFragMetadata& fragMeta) {
    uint8_t tmp;
    frame >> fragMeta.offset;
    uint16_nbo tag;
    frame >> tag;
    fragMeta.tag = tag.getUint16_t();
    frame >> tmp;
    fragMeta.lffrSeq = (tmp >> 3) & 0x1F;
    uint8_t msbOfDatagramSize = (tmp & 0x07);
    uint8_t lsbOfDatagramSize;
    frame >> lsbOfDatagramSize;
    fragMeta.dgSize = (((uint16_t)msbOfDatagramSize) << 8) | lsbOfDatagramSize;
    fragMeta.size = frame.getLength();
}


void LFFRImplementation::parseLowpanAndIpHeader(cometos::Airframe& frame,
                                uint8_t lowpanDispatch,
                                const Ieee802154MacAddress& src,
                                const Ieee802154MacAddress& dst,
                                LowpanFragMetadata& fragMeta,
                                IPv6Datagram& dg)
{
    bool retrieveIpHeader;

    if (isLFFRFragmentationHeader(lowpanDispatch)) {
        if (isRFRAG(lowpanDispatch)) {
            parseFragmentationHeader(frame, lowpanDispatch, fragMeta);
            if (fragMeta.lffrSeq == 0) {
                retrieveIpHeader = true;
            } else {
                retrieveIpHeader = false;
            }
        } else {
            // probably an RFRAG-ACK
            retrieveIpHeader = false;
            fragMeta.fragmented = false;
        }

        // retrieve next dispatchHeader
        frame >> lowpanDispatch;
    } else {
        // no fragmented datagram; fragmentation metadata is therefore invalid,
        // but we do want to get the ip header
        retrieveIpHeader = true;
        fragMeta.fragmented = false;
    }

    if (retrieveIpHeader) {
        retrieveIpBaseHeader(frame, lowpanDispatch, src, dst, dg);
    }
}

void LFFRImplementation::parseframe(cometos::Airframe& frame,
                                    uint8_t head,
                                    const Ieee802154MacAddress& src,
                                    const Ieee802154MacAddress& dst,
                                    IPv6DatagramInformation* contextInfo) {
    ASSERT(contextInfo != NULL);

    LowpanFragMetadata fragmeta;
    if (isLFFRFragmentationHeader(head)) {
        handleLFFRFragment(frame, head, src, dst, contextInfo);
    } else {
        LOG_DEBUG("No Fgmt");
        extractUnfragmentedIP(frame, head, src, dst, contextInfo);
    }
}

QueueObject* LFFRImplementation::processBufferSpaceAndReturnQueueObject(
    IPv6Request* req, uint16_t tag, BufferInformation* buf,
    uint16_t posInBuffer) {
    DatagramInformation* dGrmInfo =
            _ipRetransmissionList.addDatagram(req, buf, tag, _lowpan);
    QueueObject* tmp = NULL;
    if (dGrmInfo != NULL) {
        tmp = new LFFRPacket(dGrmInfo);
    }  // TODO: stats collection for out of _ipRetransmissionList entries
    return tmp;
}

void LFFRImplementation::handleLFFRFragment(cometos::Airframe& frame,
                                            uint8_t head,
                                            const Ieee802154MacAddress& src,
                                            const Ieee802154MacAddress& dst,
                                            IPv6DatagramInformation* contextInfo) {
    if (isRFRAG(head)) {
        handleRFRAG(frame, src, dst, contextInfo, isRFRAG_AR(head));
    } else {
        handleRFRAG_ACKMessage(frame, src, isRFRAG_AEC(head));
    }
}

void LFFRImplementation::handleRFRAG(cometos::Airframe& frame,
                                     const Ieee802154MacAddress& src,
                                     const Ieee802154MacAddress& dst,
                                     IPv6DatagramInformation* contextInfo,
                                     bool isRFRAG_AR) {
    if (!isValidFragmentFrame(frame)) return;
    LowpanFragMetadata fragMeta;
    DatagramReassembly* dgramReassemblyInfo = NULL;
    // get FragmentInformation* here and handle stuff in this function
    parseFragmentationHeader(frame, 0, fragMeta);
    if (fragMeta.lffrSeq == 0) {
        dgramReassemblyInfo =
            parseFirstFragment(frame, src, dst, fragMeta.offset,
                    fragMeta.tag, fragMeta.dgSize, isRFRAG_AR);
    } else {
        dgramReassemblyInfo = parseSubsequentFragment(
            frame, src, dst, fragMeta.offset, fragMeta.tag, fragMeta.dgSize,
            fragMeta.lffrSeq, isRFRAG_AR);
    }

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer.getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer.getNumBuffers());
#endif

    if (isReassemblyComplete(dgramReassemblyInfo)) {
        IPv6Datagram* reAssmbldDgramPtr = dgramReassemblyInfo->getDatagram();
        bool isReassembledDgramValid = (reAssmbldDgramPtr != NULL);
        if (isReassembledDgramValid) {
            contextInfo->setData(reAssmbldDgramPtr,
                                 dgramReassemblyInfo->decapsulateBuffer(),
                                 dgramReassemblyInfo->getLlRxInfo().getCopy());
        } else {
            LAL_SCALAR_INC(dropped_Invalid);
            LOG_ERROR("Datagram Missing");
        }
        dgramReassemblyInfo->free();
    }

    /*   if (isFragmentationHeaderX(head)) {
           dgramReassemblyInfo = continueReassemblyOfDgram(
               src, dGramTag.getUint16_t(), dGramSize.getUint16_t(), frame);

       } else {
           uint8_t size = frame.getLength();
           std::cout << (int)size;
           LOG_DEBUG("FragmentHeader1 Found");
           IPHCDecompressor* ipHdrWithoutCompression =
               extractDecompressedIpHeaderFrmFrame(frame, src, dst, srcContexts,
                                                   dstContexts);

           if (ipHdrWithoutCompression != NULL) {
               dgramReassemblyInfo = initiateReassemblyOfDgram(
                   src, dGramTag.getUint16_t(), dGramSize.getUint16_t(), frame,
                   ipHdrWithoutCompression);

               LOG_DEBUG("Datagram from "
                         << ipHdrWithoutCompression->getIPDatagram()->src.str()
                         << " to "
                         <<
   ipHdrWithoutCompression->getIPDatagram()->dst.str());
           } else {
               LAL_SCALAR_INC(dropped_Invalid);
               LOG_WARN("No Valid IP Datagram");
           }
       }

   #ifdef LOWPAN_ENABLE_BUFFERSTATS
       LAL_VECTOR_REC(BufferSizeVector, (double)buffer.getUsedBufferSize());
       LAL_VECTOR_REC(BufferNumElemVector, (double)buffer.getNumBuffers());
   #endif

       if (isDgramReassemblyComplete(dgramReassemblyInfo)) {
           IPv6Datagram* reAssmbldDgramPtr = dgramReassemblyInfo->getDatagram();
           bool isReassembledDgramValid = (reAssmbldDgramPtr != NULL);
           if (isReassembledDgramValid) {
               contextInfo->setData(reAssmbldDgramPtr,
                                    dgramReassemblyInfo->decapsulateBuffer(),
                                    dgramReassemblyInfo->getLlRxInfo().getCopy());
           } else {
               LAL_SCALAR_INC(dropped_Invalid);
               LOG_ERROR("Datagram Missing");
           }
           dgramReassemblyInfo->free();
       }*/
}

void LFFRImplementation::handleRFRAG_ACKMessage(cometos::Airframe& frame,
                                                const Ieee802154MacAddress& src,
                                                bool isRFRAG_AEC) {
    // get a link to the retransmission list
    uint16_nbo dataGrmTag_nbo;
    uint32_nbo ackBitmap_nbo;
    frame >> dataGrmTag_nbo;
    frame >> ackBitmap_nbo;
    uint16_t receivedTag = dataGrmTag_nbo.getUint16_t();
    uint32_t receivedAckBitmap = ackBitmap_nbo.getUint32_t();
    bufStatus_t status = BS_SUCCESS;

    if (!searchFragmentBufferAndFwdACK(receivedTag, receivedAckBitmap, src,
                                       status, isRFRAG_AEC)) {
        DatagramInformation* datagramInfoPtr =
            _ipRetransmissionList.retrieveDatagram(receivedTag, src);
        if (datagramInfoPtr != NULL) {  // ACK is addressed to this node

            LOG_DEBUG("Rx tag: " << (int)receivedTag
                      << " Received ACK bitmap: 0x"
                      << std::hex << receivedAckBitmap);

            // ACK received, stop the retransmission timer now.
            datagramInfoPtr->stopRTOTimer();

            if (receivedAckBitmap == 0x00000000) {
                // This data gram should be dropped
                LOG_DEBUG("NULL Bit Map Rcvd, Dropping IP packet");
                datagramInfoPtr->free(false);
                _lowpan->eraseQueueObject();  // remove datagram if it is still
                                              // transmitting
                return;
            }

            if (!datagramInfoPtr->areAllFragmentsAcked(receivedAckBitmap)) {
                uint32_t retransmissionList =
                    datagramInfoPtr->getRetransmissionList(receivedAckBitmap);
                LFFRPacket* qo =
                    new LFFRPacket(datagramInfoPtr, retransmissionList);
                if (!_lowpan->enqueueQueueObject(qo)) {
                    delete qo;
                    status = BS_QUEUE_FULL;
                }
            } else {
                // Free storedIp info
                datagramInfoPtr->free(true);
                _lowpan->eraseQueueObject();
            }
        } else {
            LOG_DEBUG("LFFR ACK Message Dropped - tag: " << (int)receivedTag);
        }
    }
    updateStatsOnReturnValue(status);
}


bool LFFRImplementation::isValidFragmentFrame(const cometos::Airframe& frame) {

    uint8_t dGramOffset;
    uint16_t dGramTag;
    uint16_t dGramSizeAndSeqNum;
    uint8_t expectedSizeofFragmentHeader;

    expectedSizeofFragmentHeader =
        sizeof(dGramOffset) + sizeof(dGramTag) + sizeof(dGramSizeAndSeqNum);

    if (frame.getLength() < expectedSizeofFragmentHeader)
        return false;
    else
        return true;
}

DatagramReassembly* LFFRImplementation::parseFirstFragment(
                                            cometos::Airframe& frame,
                                            const Ieee802154MacAddress& src,
                                            const Ieee802154MacAddress& dst,
                                            uint8_t dGramOffset,
                                            uint16_t dGramTag,
                                            uint16_t dGramSize,
                                            bool enableImplicitAck)
{
    DatagramReassembly* dgramReassemblyInfo = NULL;
    IPHCDecompressor* ipHdrWithoutCompression =
        getDecompressorFromFrame(frame, src, dst, srcContexts,
                                            dstContexts);
    if (ipHdrWithoutCompression != NULL) {
        LOG_DEBUG("Datagram from "
                  << ipHdrWithoutCompression->getIPDatagram()->src.str()
                  << " to "
                  << ipHdrWithoutCompression->getIPDatagram()->dst.str()
                  << " : t = " << dGramTag);

        dgramReassemblyInfo = initiateReassemblyOfDgram(
            src, dGramTag, dGramSize, frame, ipHdrWithoutCompression,
            dGramOffset, enableImplicitAck);

    } else {
        LAL_SCALAR_INC(dropped_Invalid);
        LOG_WARN("No Valid IP Datagram");
    }
    return dgramReassemblyInfo;
}

DatagramReassembly* LFFRImplementation::initiateReassemblyOfDgram(
                                const Ieee802154MacAddress& src,
                                uint16_t dGramTag,
                                uint16_t dGramSize,
                                cometos::Airframe& frame,
                                IPHCDecompressor* ipDgramWithoutHdrCompression,
                                uint8_t offset,
                                bool enableImplicitAck) {
    bufStatus_t retVal = BS_SUCCESS;
    DatagramReassembly* fi = fragmentHandler.addFirstLFFRFragment(
        src, dGramTag, dGramSize, frame, ipDgramWithoutHdrCompression, retVal,
        enableImplicitAck);
    updateStatsOnReturnValue(retVal);
    return fi;
}

DatagramReassembly* LFFRImplementation::parseSubsequentFragment(
                                                cometos::Airframe& frame,
                                                const Ieee802154MacAddress& src,
                                                const Ieee802154MacAddress& dst,
                                                uint8_t dGramOffset,
                                                uint16_t dGramTag,
                                                uint16_t dGramSize,
                                                uint8_t sequenceNumber,
                                                bool isRFRAG_AR)
{
    LOG_DEBUG("FragmentHeaderX Found|t=" << dGramTag << "|s=" << dGramSize
                                         << "|o=" << (uint16_t)dGramOffset);
    bufStatus_t retVal;
    DatagramReassembly* fi = fragmentHandler.addSubsequentLFFRFragment(
        src, dGramTag, dGramSize, dGramOffset, sequenceNumber, frame, retVal,
        isRFRAG_AR);
    updateStatsOnReturnValue(retVal);
    return fi;
}

bool LFFRImplementation::searchFragmentBufferAndFwdACK(uint16_t& receivedTag,
                                                       uint32_t& receivedAckBitmap,
                                                       const Ieee802154MacAddress& src,
                                                       bufStatus_t& status,
                                                       bool isRFRAG_AEC) {
    if (fragmentHandler.handleRFRAG_ACKMessage(
            receivedTag, src, receivedAckBitmap, status, isRFRAG_AEC)) {
        return true;
    }
    return false;
}

} /* namespace cometos_v6 */
