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

#include "IPHCCompressor.h"
#include "lowpan-macros.h"
#include "OutputStream.h"
#include "logging.h"

namespace cometos_v6 {

IPHCCompressor::~IPHCCompressor() {}

IPHCCompressor::IPHCCompressor(IPv6Datagram* datagram,
                            const Ieee802154MacAddress& srcMAC,
                            const Ieee802154MacAddress& dstMAC,
                            uint8_t typeOfUncontainedNextHeader)
        : datagram(datagram),
          srcMAC(srcMAC),
          dstMAC(dstMAC),
          nHeader(NULL),
          uncomprSize(0),
          posInCurrUncompressedHeader(0),
          typeOfUncontainedNextHeader(typeOfUncontainedNextHeader)
{
    ASSERT(datagram != NULL);
}


IPHCCompressor::IPHCCompressor(IPv6Request* req, uint8_t typeOfUncontainedNextHeader)
        : datagram(req->data.datagram),
          srcMAC(req->data.srcMacAddress),
          dstMAC(req->data.dstMacAddress),
          nHeader(NULL),
          uncomprSize(0),
          posInCurrUncompressedHeader(0),
          typeOfUncontainedNextHeader(typeOfUncontainedNextHeader)
{}

uint16_t IPHCCompressor::streamDatagramToFrame(cometos::Airframe& frame,
                                                   uint8_t maxSize,
                                                   BufferInformation* buf,
                                                   uint16_t& posInBuf)
{
    uint8_t headerBuffer[maxSize];
    bool isFragmented = false;
    uint8_t currFrameLen = 0;
    LOG_DEBUG("Beginning compression; maxSize=" << (int) maxSize
               << "|buf->getSize()=" << (int) buf->getSize()
               << "|posInBuf=" << (int) posInBuf);
    if (nHeader == NULL && uncomprSize == 0) {
        currFrameLen = putCompressibleHeadersInBuffer(headerBuffer,
                                                      maxSize,
                                                      nHeader,
                                                      LOWPAN_FRAG_HEADER1_LENGTH
//                                                      ,onlyIP
                                                      );
        if ( (nHeader != NULL
                && (nHeader->getSize()+nHeader->getUpperLayerPayloadLength() > diff(maxSize, uncomprSize)))
            || (diff(maxSize,currFrameLen + LOWPAN_FRAG_HEADER1_LENGTH) < buf->getSize() - posInBuf))
        {
            // there are headers left that do not fit into the given frame,
            // or the overall size of data for the frame is too big
            isFragmented = true;
        } else {
            isFragmented = false;
        }
    }

    if (isFragmented) {
        maxSize -= LOWPAN_FRAG_HEADER1_LENGTH;
    }

    LOG_DEBUG(cometos::dec << "written " << (int) currFrameLen << " compressed header bytes to buf");

//    bool isThisASubsequentFrameWithDatagramHeaderInfo = (!onlyIP) && (currFrameLen == 0);
    bool isThisASubsequentFrameWithDatagramHeaderInfo = currFrameLen == 0;

    if(isThisASubsequentFrameWithDatagramHeaderInfo){
        maxSize -= LOWPAN_FRAG_HEADERX_LENGTH;
    }

    LOG_DEBUG("Checking for uncompressed header;"
            << "|nHeader=" << nHeader
            <<"|maxSize=" << (int) maxSize
            << "|currFrameLen=" << (int) currFrameLen);

    // put in the headers that are NOT going to be compressed; if the
    // header does not fit, we stop here but remember the position we
    // have to carry on with the next time
    while(nHeader != NULL
            && diff(maxSize, currFrameLen) >= LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT) {

        uint16_t size;
        size = nHeader->getSize();
        uint8_t tmp[size];
        nHeader->writeHeaderToBuffer(tmp);
        while ((posInCurrUncompressedHeader < size) &&
                ((maxSize - currFrameLen) >= LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT)) {
            for (uint8_t i = 0; i < LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT; i++) {
                headerBuffer[currFrameLen++] = tmp[posInCurrUncompressedHeader++];
            }
            uncomprSize += LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT;
        }
        LOG_DEBUG("Written uncompr. header to tmp buf; written= "
                        << posInCurrUncompressedHeader
                        << "|totalSize= " << size);
        if (posInCurrUncompressedHeader == size) {
            nHeader = nHeader->getNextHeader();
            posInCurrUncompressedHeader = 0;
        }
    }

    // put in the data, which comes after all the headers have bee added
    if (nHeader == NULL
            && ((maxSize - currFrameLen) >= LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT)) {
        if (posInBuf > 0) {
            buf->freeBegin(posInBuf);
            posInBuf = 0;
        }
        // TODO This can be done more efficient. Don't copy first into buffer
        // and then into frame...
#ifdef ENABLE_LOGGING
        uint16_t bytesWritten = currFrameLen;
#endif
        LOG_DEBUG("Trying to write more bytes; maxsize=" << (int)maxSize << "|buf->getSize()=" <<(int) buf->getSize());
        while ( ((diff(maxSize, currFrameLen) > (LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT-1))
                     || (buf->getSize() < diff(maxSize, currFrameLen)))
                && (buf->getSize() > 0)) {
            LOG_DEBUG("Writing 8 bytes to frame; uncomprSize=" << (int) uncomprSize
                    << "|currFrameLen=" << (int) currFrameLen);
            for (uint8_t i = 0; (i < LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT) && (buf->getSize() > 0); i++) {
                headerBuffer[currFrameLen++] = buf->streamOut();
                uncomprSize++;
            }
        }
        LOG_DEBUG("Written " << (int) (currFrameLen-bytesWritten) << " bytes of data to frame");
    }
    maxSize -= currFrameLen;
    while (currFrameLen > 0) {
        currFrameLen--;
        frame << headerBuffer[currFrameLen];
    }
    return uncomprSize;
}

uint8_t IPHCCompressor::putCompressibleHeadersInBuffer(
        uint8_t* buffer,
        uint8_t bufferSize,
        FollowingHeader* & unCompressedNextHeader,
        uint8_t spaceForLowpanHeader) {

    LOG_DEBUG("Start header compr.: bufsize=" << (int) bufferSize << "|lowpanHeaderSize=" << (int) spaceForLowpanHeader);

    /** used as position pointer and size counter */
    uint8_t compressedSize = 0;

    buffer[compressedSize++] = IPHC_HEADER_DISPATCH;
    buffer[compressedSize++] = 0;
    uncomprSize = IPv6Datagram::IPV6_HEADER_SIZE;

    // set Context
    // TODO: We don't implement this, yet.

    // encode Flow Label and Traffic Class
    comprTCFL(buffer, compressedSize);

//    ASSERT(datagram->getNumNextHeaders() > 0);
    nHeader = (FollowingHeader*)datagram->getNextHeader();
    bool compressNextHeader = false;
    if (nHeader != NULL) {
        uint8_t type = nHeader->getHeaderType();

        // check if next header can be compressed
        bool nhIsCompressible = headerIsCompressible(type);

        // check if space in buffer is enough for whole dg -- we only compress
        // any next headers, if the whole datagram fits into the given buf
        bool bufferBigEnough = (bufferSize - uncomprSize) > nHeader->getSize() + nHeader->getUpperLayerPayloadLength();

        if (nhIsCompressible && bufferBigEnough) {
            LOG_DEBUG("Compress NH; type=" << (int) type)
            compressNextHeader = true;
            setNHCompressed(buffer);
        } else {
            LOG_DEBUG("Next header does not fit frame or is not compressible; type=" << (int) type);
            buffer[compressedSize++] = type;
        }
    } else {
        ASSERT(typeOfUncontainedNextHeader != FollowingHeader::NoNextHeaderNumber);
        LOG_DEBUG("Not available NH; type=" << (int) typeOfUncontainedNextHeader);
        buffer[compressedSize++] = typeOfUncontainedNextHeader;
    }
    // encode Hop Limit
    comprHopLimit(buffer, compressedSize);

    // encode Source Address
    comprSrcAddr(buffer, compressedSize);

    // encode Destination Address
    comprDstAddr(buffer, compressedSize);

    // now compress all next headers, as long as their uncompressed size
    // would also fit the given buffer
    while (    (nHeader != NULL)
            && (compressNextHeader)) {
        LOG_DEBUG("Compressing NH; type=" << (int) nHeader->getHeaderType());
        uncomprSize += comprNextHeader(buffer, compressedSize, bufferSize, compressNextHeader, spaceForLowpanHeader);
        nHeader = nHeader->getNextHeader();
        LOG_DEBUG("Compressed NH; uncomprSize=" << uncomprSize << "|comprSize=" << (int)compressedSize);
    }

    unCompressedNextHeader = nHeader;
    return compressedSize;
}


bool IPHCCompressor::isDone() {
    return ((uncomprSize > 0) && (nHeader == NULL));
}

void IPHCCompressor::comprTCFL(uint8_t* buf, uint8_t& pos) {
    uint8_t trafficClass = datagram->getTrafficClass();
    uint32_t flowLabel = datagram->getFlowLabel();
    if (flowLabel == 0) {
        if (trafficClass == 0) {
            buf[0] |= TF_ALL_ELIDED;
        } else {
            buf[0] |= TF_FL_ELIDED;
            buf[pos++] = trafficClass;
        }
    } else {
        if ((trafficClass & 0x3F) == 0) {
            buf[0] |= TF_DSCP_ELIDED;
            buf[pos++] = (trafficClass & 0xC0) | ((flowLabel >> 16) & 0x0F);
            buf[pos++] = ((flowLabel >> 8) & 0xFF);
            buf[pos++] = (flowLabel & 0xFF);
        } else {
            buf[0] |= TF_INLINE;
            buf[pos++] = trafficClass;
            buf[pos++] = ((flowLabel >> 16) & 0x0F);
            buf[pos++] = ((flowLabel >> 8) & 0xFF);
            buf[pos++] = (flowLabel & 0xFF);
        }
    }
}

void IPHCCompressor::comprHopLimit(uint8_t* buf, uint8_t& pos) {
    switch (datagram->getHopLimit()) {
        case 1: {
            buf[0] |= HLIM_1;
            break;
        }
        case 64: {
            buf[0] |= HLIM_64;
            break;
        }
        case 255: {
            buf[0] |= HLIM_255;
            break;
        }
        default: {
            buf[0] |= HLIM_INLINE;
            buf[pos++] = datagram->getHopLimit();
            break;
        }
    }
}

void IPHCCompressor::comprSrcAddr(uint8_t* buf, uint8_t& pos) {
    // For now we just implement the stateless compression
    // TODO We also don't use the 16 bit Mac Address

    if (datagram->src.isLinkLocal()) {
        IPv6Address own(0, 0, 0, 0, srcMAC.a1(), srcMAC.a2(), srcMAC.a3(),
                        srcMAC.a4());
        if (datagram->src.getSuffix(64) == own) {
            // The packet is from me, so we can elide everything!
            buf[1] |= SAM_ELIDED << SAM_SHIFT;
        } else {
            // We have to store the suffix in the header
            buf[1] |= SAM_8BYTE << SAM_SHIFT;
            storeIPAddress(datagram->src, 4, buf, pos);
        }
    } else if (datagram->src.isUnspecified()) {
        setContextBasedSAC(buf);
    } else {
        // As we use no stateful compression we can't compress addresses that
        // are not linklocal
        storeIPAddress(datagram->src, 0, buf, pos);
    }
}

void IPHCCompressor::comprDstAddr(uint8_t* buf, uint8_t& pos) {
    // For starters we just implement the stateless compression
    // We also don't use the 16 bit Mac Address

    if (datagram->dst.isLinkLocal()) {
        if (datagram->dst.getAddressPart(4) == dstMAC.a1() &&
                datagram->dst.getAddressPart(5) == dstMAC.a2() &&
                datagram->dst.getAddressPart(6) == dstMAC.a3() &&
                datagram->dst.getAddressPart(7) == dstMAC.a4()) {
            // The packet is from me, so we can elide everything!
            buf[1] |= DAM_ELIDED;
        } else {
            // We have to store the suffix in the header
            buf[1] |= DAM_8BYTE;
            storeIPAddress(datagram->dst, 4, buf, pos);
        }
    } else if (datagram->dst.isMulticast()) {
        setMulticast(buf);
        if (datagram->dst.getAddressPart(0) == 0xFF02) {
            // Just need one byte
            buf[1] |= DAM_M_1BYTE;
            buf[pos++] = datagram->dst.getAddressPart(7) & 0xFF;
        } else if (datagram->dst.getAddressPart(1) == 0 &&
                datagram->dst.getAddressPart(2) == 0 &&
                datagram->dst.getAddressPart(3) == 0 &&
                datagram->dst.getAddressPart(4) == 0 &&
                datagram->dst.getAddressPart(5) == 0 &&
                (datagram->dst.getAddressPart(6) & 0xFF00) == 0) {
            // Just need 4 bytes
            buf[1] |= DAM_M_4BYTE;
            buf[pos++] = datagram->dst.getAddressPart(0) & 0xFF;
            buf[pos++] = datagram->dst.getAddressPart(6) & 0xFF;
            storeIPAddress(datagram->dst, 7, buf, pos);
        } else if (datagram->dst.getAddressPart(1) == 0 &&
                datagram->dst.getAddressPart(2) == 0 &&
                datagram->dst.getAddressPart(3) == 0 &&
                datagram->dst.getAddressPart(4) == 0 &&
                (datagram->dst.getAddressPart(5) & 0xFF00) == 0) {
            // Just need 6 bytes
            buf[1] |= DAM_M_6BYTE;
            buf[pos++] = datagram->dst.getAddressPart(0) & 0xFF;
            buf[pos++] = datagram->dst.getAddressPart(5) & 0xFF;
            storeIPAddress(datagram->dst, 6, buf, pos);
        } else {
            // Everything needs to be stored...
            storeIPAddress(datagram->dst, 0, buf, pos);
        }
    } else {
        // As we use no stateful compression we can't compress addresses that
        // are not linklocal
        storeIPAddress(datagram->dst, 0, buf, pos);
    }
}

void IPHCCompressor::storeIPAddress(const IPv6Address& address,
                                             uint8_t start, uint8_t* buf,
                                             uint8_t& pos) {
    while (start < 8) {
        uint16_t part = address.getAddressPart(start++);
        buf[pos++] = part >> 8;
        buf[pos++] = part & 0xFF;
    }
}

uint16_t IPHCCompressor::comprNextHeader(
                                uint8_t* buf,
                                uint8_t& pos,
                                uint8_t maxSize,
                                bool& alsoCompressNextHeader,
                                uint8_t spaceForLowpanHeader) {
    uint8_t type = nHeader->getHeaderType();

    uint16_t size = nHeader->getSize();

    // TODO should be possible to reduce code duplication by making use of
    // polymorphism of headers here

    switch (type) {
        case IPv6HopByHopOptionsHeader::HeaderNumber: {
            uint8_t headerSize =
                    (((IPv6HopByHopOptionsHeader*)nHeader)->getHDataLength()) &
                    0xFF;

            // See if next Header can also be compressed
            alsoCompressNextHeader = writeNHCNextheader(
                    nHeader->getNextHeader(),
                    LOWPAN_NHC_EID_HOP_BY_HOP_OPTION,
                    buf,
                    pos,
                    maxSize,
                    spaceForLowpanHeader);

            buf[pos++] = headerSize;
            for (uint8_t i = 0; i < headerSize; i++) {
                buf[pos++] =
                        ((IPv6HopByHopOptionsHeader*)nHeader)->getHData()[i];
            }
            break;
        }
        case IPv6RoutingHeader::HeaderNumber: {
            uint8_t headerDataLen =
                    (((IPv6RoutingHeader*)nHeader)->getSize() - 2) & 0xFF;

            // See if next Header can also be compressed
            alsoCompressNextHeader = writeNHCNextheader(
                    nHeader->getNextHeader(),
                    LOWPAN_NHC_EID_ROUTING,
                    buf,
                    pos,
                    maxSize,
                    spaceForLowpanHeader);

            buf[pos++] = headerDataLen;
            buf[pos++] = ((IPv6RoutingHeader*)nHeader)->getRoutingType();
            buf[pos++] = ((IPv6RoutingHeader*)nHeader)->getSegmentsLeft();

            for (uint8_t i = 0;
                    i < ((IPv6RoutingHeader*)nHeader)->getHDataLength(); i++) {
                buf[pos++] = ((IPv6RoutingHeader*)nHeader)->getHData()[i];
            }
            break;
        }
        case IPv6FragmentHeader::HeaderNumber: {
            uint32_t id = ((IPv6FragmentHeader*)nHeader)->getIdentification();
            uint16_t fOffset =
                    ((IPv6FragmentHeader*)nHeader)->getFragmentOffset();

            // See if next Header can also be compressed
            alsoCompressNextHeader = writeNHCNextheader(
                    nHeader->getNextHeader(),
                    LOWPAN_NHC_EID_FRAGMENTATION,
                    buf,
                    pos,
                    maxSize,
                    spaceForLowpanHeader);

            buf[pos++] = 0;
            buf[pos++] = (uint8_t)(fOffset >> 5);
            buf[pos++] =
                    (uint8_t)(((fOffset << 3) & 0xFC) |
                            (((IPv6FragmentHeader*)nHeader)->getMFlag() & 0x01));
            buf[pos++] = id >> 24;  // 32 Bit
            buf[pos++] = id >> 16;  // 32 Bit
            buf[pos++] = id >> 8;   // 32 Bit
            buf[pos++] = id;        // 32 Bit
            break;
        }
        case IPv6DestinationOptionsHeader::HeaderNumber: {
            uint8_t headerDataLen =
                    (((IPv6DestinationOptionsHeader*)nHeader)->getHDataLength()) &
                    0xFF;

            // See if next Header can also be compressed
            alsoCompressNextHeader = writeNHCNextheader(
                    nHeader->getNextHeader(),
                    LOWPAN_NHC_EID_DESTINATION_OPTION,
                    buf,
                    pos,
                    maxSize,
                    spaceForLowpanHeader);

            buf[pos++] = headerDataLen;
            for (uint8_t i = 0; i < headerDataLen; i++) {
                buf[pos++] =
                        ((IPv6DestinationOptionsHeader*)nHeader)->getHData()[i];
            }
            break;
        }
        case UDPPacket::HeaderNumber: {
            setUDPHeader((UDPPacket*)nHeader, buf, pos, maxSize);
//            size = nHeader->getHeaderSize(); getSize now does the same for the UDP header as for other headers
            alsoCompressNextHeader = false;
            break;
        }
    }

    return size;
}

bool IPHCCompressor::writeNHCNextheader(const FollowingHeader* next,
                                                   uint8_t  nhcDispatch,
                                                   uint8_t* buf,
                                                   uint8_t& pos,
                                                   uint8_t  maxSize,
                                                   uint8_t  spaceForLowpanHeader) {
    if (next == NULL) {
        ASSERT(false);
        return false;
    }
    if (headerIsCompressible(next->getHeaderType()) &&
            (next->getSize() < diff(maxSize, pos + spaceForLowpanHeader))) {

        // next header will also be in LOWPAN_NHC format, set flag accordingly
        buf[pos++] = nhcDispatch | 0x01;
        return true;
    } else {

        // next header not compressed, type field is inlined
        buf[pos++] = nhcDispatch;
        buf[pos++] = next->getHeaderType();
        return false;
    }
}

void IPHCCompressor::setUDPHeader(UDPPacket* udpPacket, uint8_t* buf,
                                           uint8_t& pos, uint8_t maxSize) {
    uint16_t srcPort = udpPacket->getSrcPort();
    uint16_t dstPort = udpPacket->getDestPort();

    uint8_t oldPos = pos;

    // Encode the header type
    uint8_t cHeader = 0xF0;
    if (!udpPacket->isChecksumSet()) {
        cHeader |= 0x04;
    }
    uint8_t* nhcByte = &buf[pos];
    buf[pos++] = cHeader;
    if (((srcPort & 0xFFF0) == 0xF0B0) && ((dstPort & 0xFFF0) == 0xF0B0)) {
        *nhcByte |= UDP_SHORT_PORTS;
        buf[pos++] = ((srcPort & 0x0F) << 4) | (dstPort & 0x0F);
    } else if (((srcPort & 0xFF00) == 0xF000)) {
        *nhcByte |= UDP_DST_INLINE;
        buf[pos++] = srcPort & 0xFF;
        buf[pos++] = dstPort >> 8;
        buf[pos++] = dstPort & 0xFF;
    } else if (((dstPort & 0xFF00) == 0xF000)) {
        *nhcByte |= UDP_SRC_INLINE;
        buf[pos++] = srcPort >> 8;
        buf[pos++] = srcPort & 0xFF;
        buf[pos++] = dstPort & 0xFF;
    } else {
        buf[pos++] = srcPort >> 8;
        buf[pos++] = srcPort & 0xFF;
        buf[pos++] = dstPort >> 8;
        buf[pos++] = dstPort & 0xFF;
    }
    if (udpPacket->isChecksumSet()) {
        buf[pos++] = udpPacket->getChecksum() >> 8;
        buf[pos++] = udpPacket->getChecksum() & 0xFF;
    }

    LOG_DEBUG(cometos::hex << "sp=" << srcPort << "|dp=" << dstPort);
    LOG_DEBUG_PREFIX;
    for (uint8_t i = oldPos; i<pos; i++) {
        LOG_DEBUG_PURE(cometos::hex << (int) buf[i] << " ");
    }
    LOG_DEBUG_PURE(cometos::endl);
}

/*void IPHCCompressor::putUncompressedDataInBuffer(uint8_t* buffer,
                                                          FollowingHeader* firstUnCompressedHeader,
                                                          uint8_t blockSize,
                                                          uint16_t positionInHeader) {
    uint8_t size;
    if (firstUnCompressedHeader->getHeaderType() != UDPPacket::HeaderNumber) {
        size = firstUnCompressedHeader->getSize();
    } else {
        size = firstUnCompressedHeader->getHeaderSize();
    }
    uint8_t tmp[size];
    uint16_t endLocation = positionInHeader + blockSize;
    for(uint8_t i = 0; i < blockSize; i++){
        buffer[i] = tmp[positionInHeader + i];
    }
}*/

uint8_t IPHCCompressor::diff(uint8_t minuend, uint8_t subtrahend) {
    ASSERT(minuend > subtrahend);
    return minuend - subtrahend;
}


} /* namespace cometos_v6 */


