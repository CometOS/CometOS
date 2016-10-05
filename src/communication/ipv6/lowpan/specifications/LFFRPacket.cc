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

#include "LFFRPacket.h"
namespace cometos_v6 {

LFFRPacket::~LFFRPacket() {
    //__datagramInformation->
}

LFFRPacket::LFFRPacket(DatagramInformation* datagramInformation,
                       uint32_t transmissionList)
    : INVALID_VALUE(0xFF),
      _datagramInformation(datagramInformation),
      _isFragmented(false),
      _transmissionList(transmissionList) {}

void LFFRPacket::createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragMeta,
                             const IPv6Datagram* & dg) {

    ASSERT(_datagramInformation->isFree() == false);
    uint8_t sequenceNumber = getNextSeqNumForTransmission();
    LOG_DEBUG("Sending Fragment: " << (int)sequenceNumber);
    bool transmissionListEmpty = (sequenceNumber == INVALID_VALUE);
    if(transmissionListEmpty) return;

    bool isLastFragmentOfDatagram = false;
    createFragment(frame,
                   maxSize,
                   sequenceNumber,
                   isLastFragmentOfDatagram,
                   fragMeta,
                   dg);
}

QueueObject::response_t LFFRPacket::response(bool success,
                                             const cometos::MacTxInfo& info) {

    uint8_t transmittedSequenceNumber =
        getNextSeqNumForTransmission();  // This will be the fragment seq number
                                         // that was transmitted now, Since the
                                         // seq num was not cleared in
                                         // createFrame
    if (transmittedSequenceNumber == 0 &&
        _datagramInformation->hasInfo() == false) {
        return QueueObject::QUEUE_KEEP;  // This situation is when the response
                                         // from the mac was for the previous
                                         // queue object(queueFrame), which has
                                         // now been removed because resources
                                         // associated with it have been freed
    }
    bool isLastFragmentToTransmit =
        (transmittedSequenceNumber == INVALID_VALUE);
    if (!isLastFragmentToTransmit) {
        clearSeqNumFrmTransmissionList(transmittedSequenceNumber);
    }
    LOG_DEBUG(
        "LFFRPacket response tx seqNo: " << (int)transmittedSequenceNumber);
    if (transmittedSequenceNumber == 0 && success == false) {
        _datagramInformation->free(success);
        // return QueueObject::QUEUE_DELETE_OBJECT;
        return QueueObject::QUEUE_DELETE_SIMILAR;
    } else if (transmittedSequenceNumber == INVALID_VALUE &&
               _isFragmented == false) {  // The datagram does not need
                                          // fragmentation. In this case this
                                          // does not have to be retained in the
                                          // retransmission list
        if (success) {
            _datagramInformation->free(success);
            return QueueObject::QUEUE_DELETE_SIMILAR;
        } else {
            _datagramInformation->free(success);
            return QueueObject::QUEUE_DELETE_SIMILAR;
        }
        // this is the last fragment
    } else if (transmittedSequenceNumber == INVALID_VALUE &&
               _isFragmented == true) {
        return QueueObject::QUEUE_DELETE_SIMILAR;
    }
    return QueueObject::QUEUE_KEEP;
}

const Ieee802154MacAddress& cometos_v6::LFFRPacket::getDstMAC() const {
    return _datagramInformation->getdstMac();
}

bool LFFRPacket::createFragment(cometos::Airframe& frame,
                                uint8_t maxSize,
                                uint8_t sequenceNumber,
                                bool& isLastFragmentOfDatagram,
                                LowpanFragMetadata& fragMeta,
                                const IPv6Datagram* & dg)
{
    ASSERT(frame.getLength() == 0);
    bool isFragmented = false;
    bool triggerImplicitAck = false;
    uint8_t offset = populateDataGramIntoFrame(
        frame, sequenceNumber, maxSize, isLastFragmentOfDatagram, isFragmented);
    _isFragmented = isFragmented;

    // assume that if the last fragment is at its initial value, this is
    // the first time we try to send this datagram
    if (sequenceNumber == 0 && _datagramInformation->getLastFragment() == 0) {
        fragMeta.isFirst = true;
    } else {
        fragMeta.isFirst = false;
    }

    if (frame.getLength() == 0)  // error - will not be sent to mac
        return false;
    else if (!isFragmented &&
             isLastFragmentOfDatagram) {  // ip packet fits in one mac frame
        clearTransmissionlist();  // we are done, no more fragments in this case
        _datagramInformation->setLastFragment(sequenceNumber);
        return true;
    } else if (isFragmented && isLastFragmentOfDatagram) {
        clearTransmissionlist();
        _datagramInformation->setLastFragment(sequenceNumber);
        _datagramInformation->startRTOTimer();
        triggerImplicitAck = true;
    } else if (isLastFragmentToTransmit(sequenceNumber)) {
        clearTransmissionlist();
        _datagramInformation->startRTOTimer();
        triggerImplicitAck = true;
    } /* else {
         triggerImplicitAck = false;
     }*/



    ASSERT(_datagramInformation->hasInfo());
    uint16_nbo dGramtag = _datagramInformation->getTag();
    // uint16_t dGramSize = _datagramInformation->getSizeOfCompressedDatagram();
//    uint16_t dGramSize = _datagramInformation->getDatagram()->getSize();  deprecated, getSize now works differently!
    dg = _datagramInformation->getDatagram();
    uint16_t dGramSize = dg->getUpperLayerPayloadLength() + dg->getCompleteHeaderLength();
    fragMeta.dgSize = dGramSize;
    // TODO here we take the compressed size, which is different from all other QueueObjects!!!!
    fragMeta.size = frame.getLength();
    fragMeta.offset = offset;
    fragMeta.tag = dGramtag.getUint16_t();

    if (offset > highestOffset) {
        highestOffset = offset;
    }

    addLFFRHeader(frame, dGramSize, sequenceNumber, dGramtag, offset,
                  triggerImplicitAck);
    return true;
}

uint8_t LFFRPacket::populateDataGramIntoFrame(cometos::Airframe& frame,
                                              uint8_t sequenceNumber,
                                              uint8_t maxSize,
                                              bool& isLastFrame,
                                              bool& isFragmented) {
    if (sequenceNumber == 0) {
        return fetchDataForFirstFrame(frame, maxSize, isLastFrame,
                                      isFragmented);
    } else {
        uint8_t blockSize;
        isFragmented = true;
        blockSize = (maxSize - SIZE_OF_LFFR_HEADERS) &
                    0xF8;  // Making sure blockSize is a multiple of 8
        return fetchDataForSubsequentFrame(frame, sequenceNumber, blockSize,
                                           isLastFrame);
    }
}

void LFFRPacket::addHeaderDataToFrame(cometos::Airframe& frame,
                                      uint8_t* compressedHeader,
                                      uint8_t sizeOfCompressedHeader) {
    uint8_t i;
    for (i = --sizeOfCompressedHeader; i > 0; i--) {
        frame << compressedHeader[i];
    }
    frame << compressedHeader[i];
}

uint8_t LFFRPacket::getNextSeqNumForTransmission() {
    uint8_t sequenceNumber;
    for (sequenceNumber = 0; sequenceNumber < 32; sequenceNumber++) {
        bool foundNextSeqNum =
            (((_transmissionList) & (0x80000000 >> sequenceNumber)) != 0);
        if (foundNextSeqNum) return sequenceNumber;
    }
    return INVALID_VALUE;
}

void LFFRPacket::clearSeqNumFrmTransmissionList(uint8_t sequenceNumber) {
    uint32_t mask = ~((0x80000000) >> sequenceNumber);
    _transmissionList &= mask;
}

uint8_t LFFRPacket::fetchDataForFirstFrame(cometos::Airframe& frame,
                                           uint8_t maxSize, bool& isLastFrame,
                                           bool& isFragmented) {

    IPHCCompressor compressedDatagram(
        _datagramInformation->getDatagram(), _datagramInformation->getsrcMac(),
        _datagramInformation->getdstMac(),
        FollowingHeader::NoNextHeaderNumber);

    uint8_t bufferForCompressedHeader[maxSize];
    uint8_t blockSize = maxSize;
    FollowingHeader* firstUncompressedHeader = NULL;

    uint8_t sizeofCompressedHeaders =
        compressedDatagram.putCompressibleHeadersInBuffer(
            bufferForCompressedHeader, blockSize, firstUncompressedHeader,
            SIZE_OF_LFFR_HEADERS);
    uint16_t sizeOfUnCompressedHeader =
        compressedDatagram.getUncompHeaderSize();


    if (firstUncompressedHeader != NULL) {
        uint16_t remainingHeaderSize = firstUncompressedHeader->getSize();
        uint16_t payloadSize = firstUncompressedHeader->getUpperLayerPayloadLength();
        uint16_t reaminingTotalSize = remainingHeaderSize + payloadSize;
        if (reaminingTotalSize > maxSize - sizeOfUnCompressedHeader) {
            isFragmented = true;
        }
    }

    if (isFragmented) {
        blockSize = (maxSize - (uint8_t)SIZE_OF_LFFR_HEADERS);
    }

    uint8_t sizeOfUnCompHeaderPutInBuffer = 0;

    if (firstUncompressedHeader != NULL) {
        uint8_t spaceLeftInFrame =
            (blockSize - sizeofCompressedHeaders) & 0xF8;  // make multiple of 8
        sizeOfUnCompHeaderPutInBuffer = putUncompressedHeaderDataInBuffer(
            (bufferForCompressedHeader + sizeofCompressedHeaders),
            spaceLeftInFrame, firstUncompressedHeader);
    }
    uint16_t totalSizeOfHeaders =
        (sizeOfUnCompHeaderPutInBuffer + sizeofCompressedHeaders);
    uint16_t spaceRemainingInFrame = (blockSize - totalSizeOfHeaders) & 0xF8;
    BufferInformation* datagramPayload =
        _datagramInformation->getPayLoadBuffer();
    datagramPayload->populateFrame(frame, 0, spaceRemainingInFrame);
    addHeaderDataToFrame(frame, bufferForCompressedHeader, totalSizeOfHeaders);
    uint8_t sizeOfFirstFragment = frame.getLength();
    updateDatagramCompressionInfo(sizeofCompressedHeaders,
                                  firstUncompressedHeader, sizeOfFirstFragment,
                                  sizeOfUnCompressedHeader);

    uint16_t sizeOfCompressedDatagram =
        _datagramInformation->getSizeOfCompressedDatagram();
    if (sizeOfCompressedDatagram <= maxSize) isLastFrame = true;
    return 0;
}

uint16_t LFFRPacket::putUncompressedHeaderDataInBuffer(
    uint8_t* buffer, uint8_t bufferSize, FollowingHeader* unCompressedHeader,
    uint16_t startPosInHeader) {

    if ((bufferSize == 0) || (unCompressedHeader == NULL)) {
        return 0;
    }

    uint16_t sizeOfHeader;
    uint16_t bufferIndex = 0;
//    if (unCompressedHeader->getNextHeader() != NULL) {
        sizeOfHeader = unCompressedHeader->getSize();
//    } else {
//        sizeOfHeader = unCompressedHeader->getHeaderSize();
//    }
    uint8_t contentOfHeader[sizeOfHeader];
    unCompressedHeader->writeHeaderToBuffer(contentOfHeader);
    bool isThereSpaceForAnotherHeader = false;
    uint16_t numberOfBytesToCopyFromHeader = (sizeOfHeader - startPosInHeader);
    if (bufferSize > numberOfBytesToCopyFromHeader) {
        isThereSpaceForAnotherHeader = true;
    } else {
        numberOfBytesToCopyFromHeader = bufferSize;
    }

    while (bufferIndex < numberOfBytesToCopyFromHeader) {
        buffer[bufferIndex++] = contentOfHeader[startPosInHeader++];
    }
    if (isThereSpaceForAnotherHeader) {
        FollowingHeader* nextUnCompressedHeader =
            unCompressedHeader->getNextHeader();
        uint8_t* newBuffer = (buffer + bufferIndex);
        uint8_t newBufferSize = (bufferSize - bufferIndex);
        bufferIndex += putUncompressedHeaderDataInBuffer(
            newBuffer, newBufferSize, nextUnCompressedHeader);
    }
    return bufferIndex;
}

uint16_t LFFRPacket::getCompDgramSize(
    uint8_t sizeofCompressedHeaders, FollowingHeader* firstUncompressedHeader) {
    return (sizeofCompressedHeaders +
            getSizeOfUncompressedPart(firstUncompressedHeader));
}

uint16_t LFFRPacket::getSizeOfUncompressedPart(
    FollowingHeader* firstUncompressedHeader) {
    IPv6Datagram* datagram = _datagramInformation->getDatagram();
    uint16_t unCompressedSize =
        getsizeOfUncompressedHeaders(firstUncompressedHeader) +
        datagram->getUpperLayerPayloadLength();
    return unCompressedSize;
}

uint16_t LFFRPacket::getsizeOfUncompressedHeaders(
    FollowingHeader* firstUncompressedHeader) {
    if (firstUncompressedHeader == NULL) return 0;
    uint16_t sizeOfCompressedHeaders = 0;
    while (firstUncompressedHeader != NULL) {
//        if (firstUnCompressedHeader->getNextHeader() != NULL) {
            sizeOfCompressedHeaders += firstUncompressedHeader->getSize();
//        } else {
//            sizeOfCompressedHeaders += firstUnCompressedHeader->getHeaderSize();
//        }
        firstUncompressedHeader = firstUncompressedHeader->getNextHeader();
    }
    return sizeOfCompressedHeaders;
}

void LFFRPacket::updateDatagramCompressionInfo(
    uint8_t sizeofCompressedHeaders, FollowingHeader* firstUncompressedHeader,
    uint8_t sizeOfFirstFragment, uint16_t sizeOfUnCompressedHeader) {
    uint16_t sizeOfCompressedDatagram =
        getCompDgramSize(sizeofCompressedHeaders, firstUncompressedHeader);

    _datagramInformation->setCompressedHeaderSize(sizeofCompressedHeaders);
    _datagramInformation->setSizeOfCompressedDatagram(sizeOfCompressedDatagram);
    _datagramInformation->setPtrToFirstUncompressedHeader(
        firstUncompressedHeader);
    _datagramInformation->setSizeofFirstFragment(sizeOfFirstFragment);
    _datagramInformation->setSizeOfUncompressedHeader(sizeOfUnCompressedHeader);
}

uint8_t LFFRPacket::fetchDataForSubsequentFrame(cometos::Airframe& frame,
                                                uint8_t sequenceNumber,
                                                uint8_t blockSize,
                                                bool& isLastFrame) {
    ASSERT(sequenceNumber != 0);
    ASSERT(_datagramInformation->hasInfo());
    uint16_t sizeOfCompressedHeader =
        _datagramInformation->getCompressedHeaderSize();
    uint8_t sizeOfFirstFragment =
        _datagramInformation->getSizeofFirstFragment();
    uint16_t totalNumberOfBytesTransmitted =
        sizeOfFirstFragment + (sequenceNumber - 1) * blockSize;  // false
    uint16_t numberOfUnCompressedBytesTransmitted =
        totalNumberOfBytesTransmitted - sizeOfCompressedHeader;
    uint16_t compressedDgramSize =
        _datagramInformation->getSizeOfCompressedDatagram();
    FollowingHeader* relevantUncompressedHeader = NULL;
    uint16_t returnedHeaderPosition = 0;
    uint8_t sizeOfUnCompHeadersFetched = 0;
    uint16_t startPosInPayLoadBuffer = 0;
    uint16_t bytesSavedThroughCompression =
        _datagramInformation->getSizeOfUncompressedHeader() -
        sizeOfCompressedHeader;

    if ((totalNumberOfBytesTransmitted + blockSize) >= compressedDgramSize) {
        isLastFrame = true;
    }
    bool isInvalidSeqNumber =
        totalNumberOfBytesTransmitted >= compressedDgramSize;
    if (isInvalidSeqNumber) return 0;

    BufferInformation* datagramPayload;
    datagramPayload = _datagramInformation->getPayLoadBuffer();
    relevantUncompressedHeader =
        getRelevantUncompressedHeader(  // This is the first header which
                                        // overflows into the second fragment
                                        // of the datagram
            numberOfUnCompressedBytesTransmitted, returnedHeaderPosition);
    if (relevantUncompressedHeader != NULL) {
        uint8_t headerBuffer[blockSize];
        sizeOfUnCompHeadersFetched = putUncompressedHeaderDataInBuffer(
            headerBuffer, blockSize, relevantUncompressedHeader,
            returnedHeaderPosition);
        if (sizeOfUnCompHeadersFetched == blockSize) {
            addHeaderDataToFrame(frame, headerBuffer,
                                 sizeOfUnCompHeadersFetched);
            // return totalNumberOfBytesTransmitted >> 3;
            return (totalNumberOfBytesTransmitted +
                    bytesSavedThroughCompression) >>
                   3;
        } else {
            ASSERT(blockSize > sizeOfUnCompHeadersFetched);
            uint8_t spaceRemainingInBuffer =
                blockSize - sizeOfUnCompHeadersFetched;
            datagramPayload->populateFrame(frame, startPosInPayLoadBuffer,
                                           spaceRemainingInBuffer);
            addHeaderDataToFrame(frame, headerBuffer,
                                 sizeOfUnCompHeadersFetched);
            // return totalNumberOfBytesTransmitted >> 3;
            return (totalNumberOfBytesTransmitted +
                    bytesSavedThroughCompression) >>
                   3;
        }
    }
    uint16_t sizeOfUncompressedHeaders = getsizeOfUncompressedHeaders(
        _datagramInformation->getPtrToFirstUncompressedHeader());
    uint16_t sizeOfHeaderInformation =
        sizeOfCompressedHeader + sizeOfUncompressedHeaders;
    startPosInPayLoadBuffer =
        totalNumberOfBytesTransmitted - sizeOfHeaderInformation;
    datagramPayload->populateFrame(frame, startPosInPayLoadBuffer, blockSize);
    // return totalNumberOfBytesTransmitted >> 3;
    return (totalNumberOfBytesTransmitted + bytesSavedThroughCompression) >> 3;
}


FollowingHeader* LFFRPacket::getRelevantUncompressedHeader(
    uint16_t unCompressedrBytesAlreadyTransmitted,
    uint16_t& positionInReturnedHeader) {
    ASSERT(_datagramInformation->hasInfo());
    FollowingHeader* nextUncompressedHeader =
        _datagramInformation->getPtrToFirstUncompressedHeader();
    uint16_t cumilativeSize = 0;
    positionInReturnedHeader = 0;
    uint16_t currentHheaderSize;
    if (nextUncompressedHeader == NULL) {
        return NULL;
    }
//    if (nextUncompressedHeader->getNextHeader() != NULL) {
    // this was done to get the header size only in case the header was
    // a terminating header
        currentHheaderSize = nextUncompressedHeader->getSize();
//    } else {
//        currentHheaderSize = nextUncompressedHeader->getHeaderSize();
//    }
    cumilativeSize += currentHheaderSize;
    while (cumilativeSize < unCompressedrBytesAlreadyTransmitted) {
        nextUncompressedHeader = nextUncompressedHeader->getNextHeader();
        if (!nextUncompressedHeader) return NULL;
//        if (nextUncompressedHeader->getNextHeader() != NULL) {
            currentHheaderSize = nextUncompressedHeader->getSize();
//        } else {
//            currentHheaderSize = nextUncompressedHeader->getHeaderSize();
//        }
        cumilativeSize += currentHheaderSize;
    }
    // not used when nextUncompressedHeader == NULL
    uint16_t sizeTillCurrentHeader = cumilativeSize - currentHheaderSize;
    positionInReturnedHeader =
        unCompressedrBytesAlreadyTransmitted - sizeTillCurrentHeader;
    return nextUncompressedHeader;
}

bool LFFRPacket::isLastFragmentToTransmit(uint8_t sequenceNumber) {
    ASSERT(sequenceNumber < 32);
    // This is false if any bit position after theseqNum pos is 1
    for (uint8_t index = sequenceNumber + 1; index < 32; index++) {
        if ((_transmissionList & (0x80000000 >> index)) != 0) {
            return false;
        }
    }
    return true;
}
bool LFFRPacket::canBeDeleted() const { return _datagramInformation->isFree(); }


uint8_t LFFRPacket::getPRCValue() const {
    ASSERT(false);
    return 0;
}

uint8_t LFFRPacket::getSRCValue() const {
    ASSERT(false);
    return 0;
}

uint8_t LFFRPacket::getHRCValue() const {
    ASSERT(false);
    return 0;
}

LocalDgId LFFRPacket::getDgId(bool& valid) const {
    DatagramInformation* dgi = _datagramInformation;
    valid = dgi != NULL;
    if (valid) {
        return LocalDgId(dgi->getsrcMac(),
            dgi->getdstMac(),
            dgi->getTag(),
            dgi->getSizeOfUncompressedDatagram());
    } else {
        return LocalDgId(0xffff, 0xffff, 0, 0);
    }
}

const IPv6Datagram* LFFRPacket::getDg() const {
    return _datagramInformation->getipRequest()->data.datagram;
}

uint8_t LFFRPacket::currOffset() const {
    return highestOffset;
}

uint16_t LFFRPacket::getCurrDgSize() const {
    IPv6Datagram* dg = _datagramInformation->getDatagram();
    return dg->getUpperLayerPayloadLength() + dg->getCompleteHeaderLength();
}

bool LFFRPacket::belongsTo(
        Ieee802154MacAddress const & src,
        Ieee802154MacAddress const & dst,
        uint16_t tag,
        uint16_t size) const {
    DatagramInformation * dgi = _datagramInformation;
    return src == dgi->getsrcMac()
            && dst == dgi->getdstMac()
            && tag == dgi->getTag()
            && size == dgi->getSizeOfUncompressedDatagram();
}

} /* namespace cometos_v6 */
