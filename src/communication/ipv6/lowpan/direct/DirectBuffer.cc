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
#include "DirectBuffer.h"
#include "LowpanAdaptionLayer.h"

namespace cometos_v6 {


DirectBuffer::DirectBuffer(
            PacketInformation* datagramHandlers,
            uint8_t size,
            LowpanAdaptionLayer* lowpan,
            AssemblyBufferBase* assemblyBuf,
            ManagedBuffer* buffer,
            uint16_t* tag) :
        numEntries(size),
        assembly(assemblyBuf),
        lowpan(lowpan),
        buffer(buffer),
        nexttag(tag),
        ip(NULL),
        datagramHandlers(datagramHandlers)
{}


void DirectBuffer::initialize(IpForward * ip) {
    this->ip = ip;
}

DatagramReassembly* DirectBuffer::addFragment(
           const Ieee802154MacAddress& srcMAC,
           uint16_t tag,
           uint16_t size,
           uint8_t offset,
           cometos::Airframe& frame,
           bufStatus_t & status)
{
   status = BS_SUCCESS;
   PacketInformation* pi = findCorrespondingPI(srcMAC, tag, size);


   if (NULL == pi) {
       return assembly->addFragment(srcMAC, tag, size, offset, frame, status);
   }

   // packet information object found, try to add the received new fragment
   fragmentResult_t res = pi->addFragment(offset, frame.getLength());
   if (res == PI_IN_ORDER) {
       ManagedBuffer::MbRequestStatus mbStatus;
       BufferInformation* buf = buffer->getBuffer(frame.getLength(), mbStatus);
       if (buf != NULL) {
           uint16_t pos = 0;
           buf->addFrame(frame, pos);
           ASSERT(pi->isFree() == false);
           bool last = false;
           if (pi->getSize() == pi->getTransmitted()) {
               last = true;
               status = BS_SUCCESS_PACKET;
           }
           QueueFrame* qfp = new QueueFrame(pi, buf, 0, offset, last);

           // try to queue fragment
           if (!lowpan->enqueueQueueObject(qfp)) {
               delete qfp; ///< also include buf->free();
               status = BS_QUEUE_FULL;
           } else {
               return NULL;
           }
       } else {
           if (mbStatus == ManagedBuffer::FAIL_MEMORY) {
               status = BS_OUT_OF_SPACE;
           } else if (mbStatus == ManagedBuffer::FAIL_HANDLERS) {
               status = BS_OUT_OF_HANDLERS;
           }
       }
   } else if (res == PI_ALREADY) {
       return NULL;
   } else { // res == PI_INVALID_ORDER
       status = BS_FRAGMENT_INVALID_ORDER;
   }

   // critical error occurred, so cleanup the queue from all fragments
   // belonging to this datagram
   ASSERT(pi->isFree() == false);
   LOG_DEBUG("Failed to add Fragment: " << (int) status);
   pi->setFree(false);
   lowpan->eraseQueueObject();
   return NULL;
}


DatagramReassembly* DirectBuffer::addFirstFragment(
           const Ieee802154MacAddress& srcMAC,
           uint16_t tag,
           uint16_t size,
           cometos::Airframe& frame,
           IPHCDecompressor* & decompressor,
           bufStatus_t & status) {
   status = BS_SUCCESS;
   const IPv6Address* addr = decompressor->getIPv6Dest();
   IPv6Request * ipReq = NULL;

   if (addr == NULL) {
       status = BS_DATAGRAM_INVALID;
       delete decompressor;
       decompressor = NULL;
       return NULL;
   }

   if (ip->isForMe(*addr)) {
       return assembly->addFirstFragment(srcMAC, tag, size, frame, decompressor, status);
   }

   if (decompressor->getHopLimit() == 0) { // TODO put into check function in IpForward?
       status = BS_NO_HOPS_LEFT;
       delete decompressor;
       decompressor = NULL;
       return NULL;
   }

   // TODO findCorrespondingPI should also check if a tag exists with the same
   // size, in which case we need to drop the PI with this tag, currently this
   // is only done by a timeout
   if (NULL != findCorrespondingPI(srcMAC, tag, size)) {
       delete decompressor;
       status = BS_DUPLICATE;
       return NULL;
   }
   // now pre-checks are finished, get route
   routeResult_t res = ip->crossLayerRouting(ipReq, *addr);

   if (res == RR_SUCCESS) {
       ASSERT(ipReq != NULL);

       // while crossLayerRouting returns the src MAC address of this node,
       // we actually need the src MAC address from the previous hop here,
       // so we set it back to the corresponding value
       ipReq->data.srcMacAddress=srcMAC;

       ManagedBuffer::MbRequestStatus mbStatus;

        // get a buffer for the remaining data in the frame
       BufferInformation* buf = buffer->getBuffer(frame.getLength(), mbStatus);
       if (NULL != buf) {
           // first, get data of all compressed headers, which have to be
           // existent completely in datagram
           FirstFragBufInfo ffbi = decompressor->compressedNext(frame, buf);

           uint16_t uncompressedPosInBuf = ffbi.numWrittenToBuf;

           // now, store rest of frame (more headers/payload) into buffer
           ffbi.uncompressedSize += buf->addFrame(frame, ffbi.numWrittenToBuf);
           PacketInformation* handler = getFreeHandler();
           if (NULL != handler) {
               // get all compressed headers from first fragment
               IPv6Datagram * tmpDg = decompressor->decapsulateIPDatagram();

               // (*ipReq->datagram) = std::move(*tmpDg);
               // we workaround a missing (move) assignment operator
               // here to prevent memory access violations
               ipReq->data.datagram->dst = tmpDg->dst;
               ipReq->data.datagram->src = tmpDg->src;
               ipReq->data.datagram->setFlowLabel(tmpDg->getFlowLabel());
               ipReq->data.datagram->setTrafficClass(tmpDg->getTrafficClass());
               ipReq->data.datagram->setHopLimit(tmpDg->getHopLimit());
               ipReq->data.datagram->setNextHeader(tmpDg->decapsulateAllHeaders());

               delete tmpDg;
               tmpDg = NULL;
               handler->set(ipReq,
                            tag,
                            (*nexttag)++,
                            size,
                            ffbi.uncompressedSize);
               ipReq = NULL;
               handler->getDatagram()->decrHopLimit();
               ASSERT(handler->isFree() == false);

               QueueFrame* qo = new QueueFrame(handler,
                                               buf,
                                               uncompressedPosInBuf,
                                               0,
                                               false,
                                               decompressor->getNextUncompressedHeaderType());

               if (!lowpan->enqueueQueueObject(qo)) {
                   delete qo;
                   status = BS_QUEUE_FULL;
                   handler->setFree(false);
               }
           } else {
               status = BS_OUT_OF_SPACE_TINY;
               buf->free();
           }
        } else {
            if (mbStatus == ManagedBuffer::FAIL_MEMORY) {
                status = BS_OUT_OF_SPACE;
            } else if (mbStatus == ManagedBuffer::FAIL_HANDLERS) {
                status = BS_OUT_OF_HANDLERS;
            }
        }
    } else if (res == RR_NO_ROUTE) {
        status = BS_NO_ROUTE;
    } else if (res == RR_NO_MESSAGES) {
        status = BS_OUT_OF_MESSAGES;
    }

    if (status != BS_SUCCESS) {
        if (ipReq != NULL) {
            LOG_DEBUG("Respond fail to IPv6");
            ipReq->response(new IPv6Response(ipReq, IPv6Response::IPV6_RC_FAIL_ALREADY_COUNTED));
        }
    }

    delete decompressor;
    decompressor = NULL;
    return NULL;
}

uint8_t DirectBuffer::tick() {
    uint8_t timeouts = 0;
    for (uint8_t i = 0; i < numEntries; i++) {
        if (datagramHandlers[i].checkTimeout()) {
            timeouts++;
        }
    }
    timeouts += assembly->tick();
    if (timeouts > 0) lowpan->eraseQueueObject();
    return timeouts;
}

DatagramReassembly* DirectBuffer::addFirstLFFRFragment(const Ieee802154MacAddress& srcMAC,
                                                       uint16_t tag,
                                                       uint16_t size,
                                                       cometos::Airframe& frame,
                                                       IPHCDecompressor* & dcDatagram,
                                                       bufStatus_t & status, bool enableImplicitAck) {

    // Want to reuse almost every thing inside addFirstFragment,
    // but hard to reuse without refactoring the existing code.
    // For now using boiler plate
    status = BS_SUCCESS;
    const IPv6Address* addr = dcDatagram->getIPv6Dest();
    IPv6Request * ipReq = NULL;

    if (addr == NULL) {
        status = BS_DATAGRAM_INVALID;
        delete dcDatagram;
        dcDatagram = NULL;
        return NULL;
    }

    if (ip->isForMe(*addr)) {
        DatagramReassembly* fi = assembly->addFirstLFFRFragment(srcMAC, tag, size, frame, dcDatagram, status, enableImplicitAck);
        if(enableImplicitAck && (fi != NULL)){
            LOG_DEBUG("ACK requested by fragment: " << (int)0);
            sendLFFRAck(fi, status);
        }
        return fi;
    }

    if (dcDatagram->getHopLimit() == 0) { // TODO put into check function in IpForward?
        status = BS_NO_HOPS_LEFT;
        delete dcDatagram;
        dcDatagram = NULL;
        return NULL;
    }

    if (NULL != findCorrespondingPI(srcMAC, tag, size)) {
        //ASSERT(false); // this should not happen
        // Looks like there can be duplicate messages from the
        // mac layer?.. ignore such duplicates
        delete dcDatagram;
        status = BS_DUPLICATE;
        return NULL;
    }
/*// Retransmissions are valid.. let this frag through really  if you have info on it
    PacketInformation* handler = findCorrespondingPI(srcMAC, tag, size);
    if (NULL != handler) {
        delete dcDatagram; // we should already have the routing in the handler
        handler.flagRetransmission(); // used to prevent removal
        ManagedBuffer::MbRequestStatus mbStatus;
        BufferInformation* buf = buffer->getBuffer(frame.getLength(), mbStatus);
        uint8_t sequenceNumber = 0;
        uint8_t offsetForFirstFragment = 0;
        if (buf != NULL) {
            buf->addFrame(frame);
            ASSERT(handler->isFree() == false);
            handler->resetStatus(); // refresh this already existing entry ... its in use
            LFFRFrame* qfp = new LFFRFrame(handler, buf, offsetForFirstFragment, sequenceNumber, enableImplicitAck);
            // try to queue fragment
            if (!lowpan->enqueueQueueObject(qfp)) {
                delete qfp; ///< also include buf->free();
                status = BS_QUEUE_FULL;
            } else {
                return NULL;
            }
        } else {
            if (mbStatus == ManagedBuffer::FAIL_MEMORY) {
                status = BS_OUT_OF_SPACE;
            } else if (mbStatus == ManagedBuffer::FAIL_HANDLERS) {
                status = BS_OUT_OF_HANDLERS;
            }
        }
        return NULL;
    }*/

    // now pre-checks are finished, get route
    routeResult_t res = ip->crossLayerRouting(ipReq, *addr);

    if (res == RR_SUCCESS) {
        ASSERT(ipReq != NULL);

        // while crossLayerRouting returns the src MAC address of this node,
        // we actually need the src MAC address from the previous hop here,
        // so we set it back to the corresponding value
        ipReq->data.srcMacAddress=srcMAC;

        ManagedBuffer::MbRequestStatus mbStatus;
        BufferInformation* buf = buffer->getBuffer(frame.getLength(), mbStatus);
        if (NULL != buf) {
           FirstFragBufInfo SBP = dcDatagram->compressedNext(frame, buf);

           uint16_t uncompressedPos = SBP.numWrittenToBuf;

           SBP.uncompressedSize += buf->addFrame(frame, SBP.numWrittenToBuf);
           PacketInformation* handler = getFreeHandler();
           handler = getFreeHandler();
           if (NULL != handler) {
               // we just retrieve the data
               IPv6Datagram * tmpDg = dcDatagram->decapsulateIPDatagram();

//               (*ipReq->datagram) = std::move(*tmpDg);
               // TODO FIXME we workaround a missing (move) assignment operator
               // here to prevent memory access violations
               ipReq->data.datagram->dst = tmpDg->dst;
               ipReq->data.datagram->src = tmpDg->src;
               ipReq->data.datagram->setFlowLabel(tmpDg->getFlowLabel());
               ipReq->data.datagram->setTrafficClass(tmpDg->getTrafficClass());
               ipReq->data.datagram->setHopLimit(tmpDg->getHopLimit());
               ipReq->data.datagram->setNextHeader(tmpDg->decapsulateAllHeaders());

               delete tmpDg;
               tmpDg = NULL;
               handler->set(ipReq, tag,
                       (*nexttag)++,
                       size,
                       SBP.uncompressedSize);
               ipReq = NULL;
               handler->getDatagram()->decrHopLimit();
               ASSERT(handler->isFree() == false);
               uint8_t sequenceNumber = 0;
               uint8_t offsetForFirstFragment = 0;
               handler->storeLastSeqNumberOfDatagram(sequenceNumber);
               LFFRFrame* qo = new LFFRFrame(handler,
                                             buf,
                                             uncompressedPos,
                                             offsetForFirstFragment,
                                             sequenceNumber,
                                             enableImplicitAck,
                                             dcDatagram->getNextUncompressedHeaderType());
               if (!lowpan->enqueueQueueObject(qo)) {
                   delete qo;
                   status = BS_QUEUE_FULL;
                   handler->setFree(false);
               }
           } else {
               status = BS_OUT_OF_SPACE_TINY;
               buf->free();
           }
        } else {
            if (mbStatus == ManagedBuffer::FAIL_MEMORY) {
                status = BS_OUT_OF_SPACE;
            } else if (mbStatus == ManagedBuffer::FAIL_HANDLERS) {
                status = BS_OUT_OF_HANDLERS;
            }
        }
    } else if (res == RR_NO_ROUTE) {
        status = BS_NO_ROUTE;
    } else if (res == RR_NO_MESSAGES) {
        status = BS_OUT_OF_MESSAGES;
    }

    if (status != BS_SUCCESS) {
        if (ipReq != NULL) {
            ipReq->response(new IPv6Response(ipReq, IPv6Response::IPV6_RC_FAIL_ALREADY_COUNTED));
        }
    }

    delete dcDatagram;
    dcDatagram = NULL;
    return NULL;

}

DatagramReassembly* DirectBuffer::addSubsequentLFFRFragment(
    const Ieee802154MacAddress& srcMAC, uint16_t tag, uint16_t size,
    uint8_t offset, uint8_t sequenceNumber, cometos::Airframe& frame,
    bufStatus_t& status, bool enableImplicitAck) {
    status = BS_SUCCESS;
    PacketInformation* pi = findCorrespondingPI(srcMAC, tag, size);
    if (NULL == pi) {
        DatagramReassembly* fi = assembly->addSubsequentLFFRFragment(
            srcMAC, tag, size, offset, sequenceNumber, frame, status,
            enableImplicitAck);

        if (enableImplicitAck && (fi != NULL)) {
            LOG_DEBUG("ACK requested by frgmnt: "
                      << (int)sequenceNumber << "|t: " << (int)tag
                      << " |btmap: " << std::hex << fi->getAckBitmap());
            sendLFFRAck(fi, status);
        }

        if (fi == NULL) {
            LOG_DEBUG(
                "Lookup information Not found. Fragment: "
                << (int)sequenceNumber
                << " dropped");  // Not in both direct and assembly buffers
            // send out null bitmap
            sendLFFRNullBitmap(srcMAC, tag, status);
        }

        return fi;
    }
    // pi->addFragment keeps track of size received so far, and can cause
    // the packet to be removed if out of order/is a retransmission.
    // this is not be applicable to LFFR.
    // fragmentResult_t res = pi->addFragment(offset, frame.getLength());
    pi->storeLastSeqNumberOfDatagram(sequenceNumber);
    ManagedBuffer::MbRequestStatus mbStatus;
    BufferInformation* buf = buffer->getBuffer(frame.getLength(), mbStatus);
    if (buf != NULL) {
        uint16_t pos = 0;
        buf->addFrame(frame, pos);
        ASSERT(pi->isFree() == false);
        // refresh this pi entry
        pi->resetStatus();
        LFFRFrame* qfp =
            new LFFRFrame(pi,
                          buf,
                          0,
                          offset,
                          sequenceNumber,
                          enableImplicitAck);
        // try to queue fragment
        if (!lowpan->enqueueQueueObject(qfp)) {
            delete qfp;  ///< also include buf->free();
            status = BS_QUEUE_FULL;
        } else {
            return NULL;
        }
    } else {
        if (mbStatus == ManagedBuffer::FAIL_MEMORY) {
            status = BS_OUT_OF_SPACE;
        } else if (mbStatus == ManagedBuffer::FAIL_HANDLERS) {
            status = BS_OUT_OF_HANDLERS;
        }
    }

    return NULL;
}

bool DirectBuffer::handleRFRAG_ACKMessage(uint16_t& receivedTag,
                                          const Ieee802154MacAddress& srcMAC,
                                          uint32_t& receivedAckBitmap,
                                          bufStatus_t& status,
                                          bool enableECN) {
    // do a reverse lookup here
    PacketInformation* directBufInfoOnDatagram =
        reverseLookUpPI(srcMAC, receivedTag);
    if (directBufInfoOnDatagram ==
        NULL) {  // the code will check the IPRetransmission list after this
        return false;
    }

    directBufInfoOnDatagram->resetStatus();
    uint16_t tagForNextNode = directBufInfoOnDatagram->getTag();
    Ieee802154MacAddress addressofNextNode =
        directBufInfoOnDatagram->getSrcMAC();
    LFFRAck* qfp = new LFFRAck(receivedAckBitmap, tagForNextNode,
                               addressofNextNode, enableECN);

    if (!lowpan->enqueueQueueObject(qfp)) {
        delete qfp;  ///< also include buf->free();
        status = BS_QUEUE_FULL;
        return false;
    }

    if (receivedAckBitmap ==
        NULL_ACK_BITMAP) {  // Free resources on NULL Bitmap
        directBufInfoOnDatagram->setFree(false);
        LOG_DEBUG("NULL bitmap Rcvd: Cleared Direct Buffer entry");
        // erase any the LFFR frames already in the queue
        lowpan->eraseQueueObject();
    } else if (areAllLFFRFragmentsAcked(
                   directBufInfoOnDatagram,
                   receivedAckBitmap)) {  // All the fragments for this datagram
                                          // have been acked, meaning a
                                          // successful transmission of this
                                          // datagram

        directBufInfoOnDatagram->setFree(true);
        status = BS_SUCCESS_PACKET;
        lowpan->eraseQueueObject();  // Ideally this should not be needed. But
                                     // LFFR Lowpan implementation does not have
                                     // a way of identifying MAC layer
                                     // duplicates. More than one instance of
                                     // the last fragment could be queued up.
                                     // triggering an ACK before the rest of the
                                     // duplicate LFFRFrame objects are
                                     // processed.
        LOG_DEBUG("All fragments Ackd Freeing Direct Buffer Resources");
    }
    LOG_DEBUG("Forwarded LFFR ACK Message RxTag: "
              << (int)receivedTag << " OutgoingTag: " << (int)tagForNextNode
              << " Bitmap: " << std::hex << receivedAckBitmap);
    return true;
}

PacketInformation* DirectBuffer::getFreeHandler() {
    uint8_t id = 0;
    for (; (id < numEntries) && (!(datagramHandlers[id].isFree())); id++);
    if (id < numEntries) {
        return &(datagramHandlers[id]);
    }
    return NULL;
}


PacketInformation* DirectBuffer::findCorrespondingPI(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size) {
    uint8_t id = 0;
    for (; (id < numEntries) && (
            datagramHandlers[id].isFree() ||
            datagramHandlers[id].getTag() != tag ||
            datagramHandlers[id].getSize() != size ||
            datagramHandlers[id].getSrcMAC() != srcMAC);
        id++);
    if (id < numEntries) {
        return &(datagramHandlers[id]);
    }
    return NULL;
}

PacketInformation* DirectBuffer::reverseLookUpPI(
            const Ieee802154MacAddress& receivedMac,
            uint16_t tag)
{
    uint8_t id = 0;
    for (; (id < numEntries) && (
            datagramHandlers[id].isFree() ||
            datagramHandlers[id].getNewTag() != tag ||
            datagramHandlers[id].getDstMAC()!= receivedMac);
        id++);
    if (id < numEntries) {
        return &(datagramHandlers[id]);
    }
    return NULL;
}

void DirectBuffer::sendLFFRAck(DatagramReassembly* fi, bufStatus_t& status) {
    bool enableECN = false;
    LFFRAck* qo = new LFFRAck(fi->getAckBitmap(), fi->getTag(),
                              fi->getMACAddr(), enableECN);
    if (!lowpan->enqueueQueueObject(qo)) {
        delete qo;
        status = BS_QUEUE_FULL;
    }
}
void DirectBuffer::sendLFFRNullBitmap(const Ieee802154MacAddress& dstMac,
                                      uint16_t& dstTag, bufStatus_t& status) {
    LFFRAck* qo = new LFFRAck(0x00000000, dstTag, dstMac, false);
    if (!lowpan->enqueueQueueObject(qo)) {
        delete qo;
        status = BS_QUEUE_FULL;
        return;
    }
    LOG_DEBUG(
        "Sending NULL bitmap RFRAG-ACK with tag: "
        << (int) dstTag);
}

bool DirectBuffer::areAllLFFRFragmentsAcked(
            PacketInformation* directBufInfoOnDatagram,
            const uint32_t& ackBitmap)
{
    uint8_t lastSeqNumberOfDatagram =
        directBufInfoOnDatagram->getLastSeqNumberOfDatagram();
    ASSERT(lastSeqNumberOfDatagram != INVALID_SEQ_NO);
    uint32_t expectedBitmap = generateFullyAckdBitmap(lastSeqNumberOfDatagram);
    return (expectedBitmap == ackBitmap);
}

uint32_t DirectBuffer::generateFullyAckdBitmap(uint8_t sequenceNumber) {
    ASSERT(sequenceNumber < 32);
    uint32_t bitMap = ~(0x7FFFFFFF >> sequenceNumber);
    return bitMap;
}


DynDirectBuffer::DynDirectBuffer(
            LowpanAdaptionLayer* lowpan,
            AssemblyBufferBase* assemblyBuf,
            ManagedBuffer* buffer,
            uint8_t numDatagramHandlers,
            uint16_t* tag) :
        DirectBuffer(new PacketInformation[numDatagramHandlers],
                     numDatagramHandlers,
                     lowpan,
                     assemblyBuf,
                     buffer,
                     tag)
{
}


DynDirectBuffer::~DynDirectBuffer() {
    PacketInformation*& pi = this->getDatagramHandlers();
    delete[] pi;
    pi = NULL;

}

} // namespace



