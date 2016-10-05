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

#include "IPHCDecompressor.h"
#include "lowpan-macros.h"
#include "logging.h"

namespace cometos_v6 {

const uint8_t IPHCDecompressor::IPV6_HEADER_SIZE_UNCOMPRESSED;
const uint8_t IPHCDecompressor::IPV6_COMPRESSED_HEADER_SIZE_INVALID;
const uint8_t IPHCDecompressor::LOWPAN_IPHC_MINIMUM_HEADER_SIZE;

IPHCDecompressor::IPHCDecompressor(
        uint8_t lowpanDispatch,
        cometos::Airframe& frame,
        const Ieee802154MacAddress& srcMAC,
        const Ieee802154MacAddress& dstMAC,
        const IPv6Context* srcCntxt,
        const IPv6Context* dstCntxt) :
                datagram(NULL),
                nextUncompressed(NULL),
                compressedIPv6HeaderSize(IPV6_COMPRESSED_HEADER_SIZE_INVALID)
{
    LOG_DEBUG("Start decompression; frame len: " << (int) frame.getLength());
    if (isIPHCHeader(lowpanDispatch)) {
        datagram = new IPv6Datagram();
        if (!decompressIPHeader(lowpanDispatch, frame, srcMAC, dstMAC, srcCntxt, dstCntxt)) {
            delete datagram;
            datagram = NULL;
        }
    } else if (isIPv6Uncompressed(lowpanDispatch) && frame.getLength() >= IPV6_HEADER_SIZE_UNCOMPRESSED) {
        datagram = new IPv6Datagram();
        uint8_t b[IPV6_HEADER_SIZE_UNCOMPRESSED];
        for (uint8_t i = 0; i < IPV6_HEADER_SIZE_UNCOMPRESSED; i++) {
            frame >> b[i]; // Byteorder
        }
        // we only parse the IPv6 Header and the first next header here
        datagram->parse(b, IPV6_HEADER_SIZE_UNCOMPRESSED);
    } else {
        // Header with unknown type or invalid length; datagram remains NULL
    }
}

IPHCDecompressor::~IPHCDecompressor() {
    if (datagram != NULL) {
        delete datagram;
    }
    if (nextUncompressed != NULL) {
        delete nextUncompressed;
    }
}

FirstFragBufInfo IPHCDecompressor::compressedNext(
        cometos::Airframe& frame, BufferInformation* buf)
{
    ASSERT(compressedIPv6HeaderSize != IPV6_COMPRESSED_HEADER_SIZE_INVALID);

    /** to keep track of the first fragments compressed and uncompressed size*/
    FirstFragBufInfo ffbi;
    ffbi.compressedSize = compressedIPv6HeaderSize;
    ffbi.uncompressedSize = IPV6_HEADER_SIZE_UNCOMPRESSED;

    // check if next header is set (if it is, there is nothing more to
    // decompress
//    if (datagram->getNextHeader() == NULL) {
    if (nextUncompressed == NULL) {
        bool nextCompressed = true;
        while (nextCompressed) {

            // retrieve the LOWPAN_NHC "dispatch byte"
            uint8_t nhcDispatch;
            frame >> nhcDispatch; // the nhc dispatch byte is counted later
            ffbi.compressedSize += sizeof(nhcDispatch);
            if (isExtensionHeader(nhcDispatch)) {
                getNHCEncodedExtensionHeader(nhcDispatch, frame, buf, ffbi, nextCompressed);
//                ret.uncompressedSize += getExtensionHeader(nhcDispatch, frame, buf, ret.numWrittenToBuf,
//                        nextCompressed);
            } else if (isUDPHeader(nhcDispatch)) {
                nextCompressed = false;
                getNHCEncodedUDPHeader(nhcDispatch, frame, buf, ffbi);
            } else {
                return ffbi;
            }
        }
    }
    return ffbi;
}

bool IPHCDecompressor::uncompressedNext(BufferInformation* buf,
        uint16_t& bufferPos)
{
    // Iterate over uncompressed next Headers
    while (nextUncompressed != NULL) {
        FollowingHeader* currUncompressed = nextUncompressed;
        headerType_t type = currUncompressed->getHeaderType();
        switch (type) {

        // UDP/ICMPv6 terminate the header chain; do not look for a next header
        case ICMPv6Message::HeaderNumber:
        case UDPPacket::HeaderNumber: {
        	nextUncompressed = NULL;
            break;
        }
        case IPv6DestinationOptionsHeader::HeaderNumber:
        case IPv6FragmentHeader::HeaderNumber:
        case IPv6HopByHopOptionsHeader::HeaderNumber:
        case IPv6RoutingHeader::HeaderNumber: {
            // we already got the type of the next uncompressed header in the
            // buffer in nextUncompressed;
            // now we retrieve the type of the header following this one
            LOG_DEBUG("here; " << (int) (*buf)[bufferPos]);
            FollowingHeader* tnh = IPv6Datagram::createNextHeader((*buf)[bufferPos++]);

            // one of the extension headers has always to be followed by some
            // terminating header (UDP, ICMP, TPC, etc.); if not, datagram
            // is invalid
            if (tnh == NULL) {
                return false;
            }

//            datagram->addHeader(tnh);
            nextUncompressed = tnh;
            break;
        }
        default: {
            // Unknown Header
            return false;
        }
        }
        // now we parse the current header from the buffer
        bufferPos += currUncompressed->parse((buf->getContent() + bufferPos),
                (buf->getSize() - bufferPos));
        // after having it parsed, we add the header to the datagram
        datagram->addHeader(currUncompressed);
        // if we found another next header, set it to be parsed next iteration
//        nextUncompressed = nextUncompressed->getNextHeader();
    }
    return true;
}

uint8_t IPHCDecompressor::getNextUncompressedHeaderType() {
    if (nextUncompressed != NULL) {
        return nextUncompressed->getHeaderType();
    } else {
        return FollowingHeader::NoNextHeaderNumber;
    }
}

bool IPHCDecompressor::decompressIPHeader(uint8_t iphcByte1,
        cometos::Airframe& frame,
        const Ieee802154MacAddress& srcMAC,
        const Ieee802154MacAddress& dstMAC,
        const IPv6Context* srcCntxt,
        const IPv6Context* dstCntxt) {
    uint8_t iphcByte2;

    // get second, always present byte of IPHC header
    if (frame.getLength() < sizeof(iphcByte2)) return false;
    frame >> iphcByte2; // ByteOrder

    // get Context
    contextNrs_t cnr;

    uint8_t bytesReadFromFrame = 0;
    compressedIPv6HeaderSize = LOWPAN_IPHC_MINIMUM_HEADER_SIZE;

    // now we parse the variable part of the IPHC encoding
    if (!getContext(iphcByte2, frame, cnr, bytesReadFromFrame)) return false;
    compressedIPv6HeaderSize += bytesReadFromFrame;

    // Get Flow Label and Traffic Class, ignore their values for now
    getTrafficClassFlowLabel(iphcByte1, frame, bytesReadFromFrame);
    compressedIPv6HeaderSize += bytesReadFromFrame;

    // Get Next Header, in case it is NOT compressed, but inlined;
    // setting the nextUncompressed here indicates that there is no further
    // compressed header.
    if (isNHInlineIPHC(iphcByte1)) {
        uint8_t nhType;
        if (frame.getLength() < sizeof(nhType)) return false;

        // next byte is inlined nextHeader byte, get it
        frame >> nhType;
        compressedIPv6HeaderSize += sizeof(nhType);
        LOG_DEBUG("here");
        FollowingHeader* tNH = IPv6Datagram::createNextHeader(nhType);
        if (tNH == NULL) {
            LOG_WARN("Unknown header type " << (int) nhType << "; aborting");
            return false;
        }
        nextUncompressed = tNH;
        LOG_DEBUG("Next header uncompr.; type=" << (int) nhType);
//        if (tNH != NULL) {
//            datagram->addHeader(tNH);
//        } else {
//            // FIXME: Unknown Header;
//            return false;
//        }
    }

    // Get Hop Limit
    getHopLimit(iphcByte1, frame, bytesReadFromFrame);
    compressedIPv6HeaderSize += bytesReadFromFrame;

    // Get Source Address
    getSrcAddress(iphcByte2, srcMAC, frame, srcCntxt[cnr.src], bytesReadFromFrame);
    compressedIPv6HeaderSize += bytesReadFromFrame;

    // Get Destination Address
    getDstAddress(iphcByte2, dstMAC, frame, dstCntxt[cnr.dst], bytesReadFromFrame);
    compressedIPv6HeaderSize += bytesReadFromFrame;

    LOG_DEBUG("Decompressed IPv6Header; read " << (int) compressedIPv6HeaderSize
               << " bytes from frame; remaining len: " << int(frame.getLength()));

    return true;
}

/**
 *
 */
bool IPHCDecompressor::getContext(
        uint8_t iphcByte2, cometos::Airframe& frame, contextNrs_t& cnr, uint8_t & bytesReadFromFrame) {
    bytesReadFromFrame = 0;
    if (isCIDInline(iphcByte2)) {
        // context information present, retrieve from frame
        uint8_t c;
        if (frame.getLength() < sizeof(c)) return false;
        frame >> c; // ByteOrder
        bytesReadFromFrame = 1;
        cnr.src = c >> 4;
        cnr.dst = c & 0x0F;
    }
    return true;
}

bool IPHCDecompressor::getTrafficClassFlowLabel(
        uint8_t head, cometos::Airframe& frame, uint8_t & bytesReadFromFrame)
{
    bytesReadFromFrame = 0;
    uint8_t tf = getTF(head);
    uint8_t a, b, c;
    switch (tf) {
    case TF_ALL_ELIDED: {
        datagram->setTrafficClass(0);
        break;
    }
    case TF_FL_ELIDED:
    case TF_INLINE: {
        if (frame.getLength() < sizeof(a)) return false;
        frame >> a;
        bytesReadFromFrame += sizeof(a);
        datagram->setTrafficClass(a);
        break;
    }
    case TF_DSCP_ELIDED: {
        if (frame.getLength() < sizeof(a)) return false;
        frame >> a;
        bytesReadFromFrame += sizeof(a);
        datagram->setTrafficClass(a & 0xC0);
        break;
    }
    }

    switch (tf) {
    case TF_ALL_ELIDED:
    case TF_FL_ELIDED: {
        datagram->setFlowLabel(0);
        break;
    }
    case TF_INLINE:
        if (frame.getLength() < sizeof(a)) return false;
        frame >> a;
        bytesReadFromFrame += sizeof(a);
        /* no break */
    case TF_DSCP_ELIDED: {
        if (frame.getLength() < (sizeof(b) + sizeof(c))) return false;
        frame >> b >> c;
        bytesReadFromFrame += sizeof(b) + sizeof(c);
        datagram->setFlowLabel(
                (((uint32_t) a & 0x0F) << 16)
                        | ((uint16_t) b << 8) | c);
        break;
    }
    }

    return true;
}


bool IPHCDecompressor::getHopLimit(
        uint8_t head, cometos::Airframe& frame, uint8_t & bytesReadFromFrame)
{
    static const uint8_t fixHL[4] = { 0, 1, 64, 255 };
    bytesReadFromFrame = 0;
    if (getHLim(head) == HLIM_INLINE) {
        uint8_t hl;
        if (frame.getLength() < sizeof(hl)) return false;
        frame >> hl;
        bytesReadFromFrame += sizeof(hl);
        datagram->setHopLimit(hl);
    } else {
        datagram->setHopLimit(fixHL[getHLim(head)]);
    }
    return true;
}

/**
 *
 */
IPv6Address IPHCDecompressor::getIPAddress(bool contextMode,
        uint8_t addressMode, const Ieee802154MacAddress& MACAddr,
        cometos::Airframe& frame, const IPv6Context& Cntxt, uint8_t & bytesReadFromFrame)
{
    IPv6Address ret;
    bytesReadFromFrame = 0;
    if (!contextMode) { // Stateless Compression
        switch (addressMode) {
        case AM_SL_INLINE: // Inline
            for (uint8_t i = 0; i < 8; i++) {
                uint16_nbo a;
                frame >> a;
                bytesReadFromFrame += sizeof(a);
                ret.setAddressPart(a.getUint16_t(), i);
            }
            break;
        case AM_SL_8BYTE: // 8 Byte
            ret.setLinkLocal();
            for (uint8_t i = 4; i < 8; i++) {
                uint16_nbo a;
                frame >> a;
                bytesReadFromFrame += sizeof(a);
                ret.setAddressPart(a.getUint16_t(), i);
            }
            break;
        case AM_SL_2BYTE: // 2 Byte
        {
            uint16_nbo a;
            frame >> a;
            bytesReadFromFrame += sizeof(a);
            ret.set(0xFE80, 0, 0, 0, 0, 0xFF, 0xFE00, a.getUint16_t());
            break;
        }
        case AM_SL_ELIDED: // Elided
            ret.set(0xFE80, 0, 0, 0, MACAddr.a1(), MACAddr.a2(),
                    MACAddr.a3(), MACAddr.a4());
            break;
        }
    } else { // Stateful, context-based Compression
        switch (addressMode) {
        case AM_SF_8BYTE: {
            uint16_nbo a, b, c, d;
            frame >> a >> b >> c >> d;
            bytesReadFromFrame += sizeof(a) + sizeof(b) + sizeof(c) + sizeof(d);
            ret.setContext(Cntxt,
                    a.getUint16_t(), b.getUint16_t(), c.getUint16_t(), d.getUint16_t());
            break;
        }
        case AM_SF_2BYTE: {
            uint16_nbo a;
            frame >> a;
            bytesReadFromFrame += sizeof(a);
            ret.setContext(Cntxt, 0, 0xFF, 0xFE00, a.getUint16_t());
            break;
        }
        case AM_SF_ELIDED:
            ret.setContext(Cntxt,
                    MACAddr.a1(), MACAddr.a2(), MACAddr.a3(), MACAddr.a4());
            break;
        }
    }

    return ret;
}

/**
 *
 */
bool IPHCDecompressor::getSrcAddress(
        uint8_t iphcByte2,
        const Ieee802154MacAddress& MACAddr,
        cometos::Airframe& frame,
        const IPv6Context& srcCntxt, uint8_t & bytesReadFromFrame)
{
    datagram->src = getIPAddress(isContextBasedSAC(iphcByte2), getSAM(iphcByte2),
            MACAddr, frame, srcCntxt, bytesReadFromFrame);
    return true;
}


bool IPHCDecompressor::getDstAddress(
        uint8_t iphcByte2,
        const Ieee802154MacAddress& MACAddr,
        cometos::Airframe& frame,
        const IPv6Context& dstCntxt, uint8_t & bytesReadFromFrame)
{
    bytesReadFromFrame = 0;
    uint8_t dam = getDAM(iphcByte2);
    if (!isMulticast(iphcByte2)) {
        datagram->dst = getIPAddress(isContextBasedDAC(iphcByte2), dam, MACAddr,
                frame, dstCntxt, bytesReadFromFrame);
    } else if (!isContextBasedDAC(iphcByte2) && isMulticast(iphcByte2)) {
        switch (dam) {
        case DAM_M_INLINE: {
            for (uint8_t i = 0; i < 8; i++) {
                uint16_nbo a;
                if (frame.getLength() < (sizeof(a))) return false;
                frame >> a;
                bytesReadFromFrame += sizeof(a);
                datagram->dst.setAddressPart(a.getUint16_t(), i);
            }
            break;
        }
        case DAM_M_6BYTE: {
            uint8_t a, b;
            uint16_nbo c, d;
            uint8_t toRead = sizeof(a) + sizeof(b) + sizeof(c) + sizeof(d);
            if (frame.getLength() < toRead) return false;
            frame >> a >> b >> c >> d;
            bytesReadFromFrame += toRead;
            datagram->dst.setAddressPart(0xFF00 | a, 0);
            datagram->dst.setAddressPart(b, 5);
            datagram->dst.setAddressPart(c.getUint16_t(), 6);
            datagram->dst.setAddressPart(d.getUint16_t(), 7);
            break;
        }
        case DAM_M_4BYTE: {
            uint8_t a, b;
            uint16_nbo c;
            uint8_t toRead = sizeof(a) + sizeof(b) + sizeof(c);
            if (frame.getLength() < toRead) return false;
            frame >> a >> b;
            frame >> c;
            bytesReadFromFrame += sizeof(a) + sizeof(b) + sizeof(c);
            datagram->dst.setAddressPart(0xFF00 | a, 0);
            datagram->dst.setAddressPart(b, 6);
            datagram->dst.setAddressPart(c.getUint16_t(), 7);
            break;
        }
        case DAM_M_1BYTE: {
            uint8_t a;
            if (frame.getLength() < (sizeof(a))) return false;
            frame >> a;
            bytesReadFromFrame += sizeof(a);
            datagram->dst.setAddressPart(0xFF02, 0);
            datagram->dst.setAddressPart(a, 7);
            break;
        }
        }
    } else {
        switch (dam) {
        case DAM_M_INLINE: {
            uint8_t a, b;
            uint16_nbo c, d;
            uint8_t toRead = sizeof(a) + sizeof(b) + sizeof(c) + sizeof(d);
            if (frame.getLength() < toRead) return false;
            frame >> a >> b >> c >> d;
            bytesReadFromFrame += toRead;
            datagram->dst.set(
                    0xFF00 | a,
                    ((uint16_t) b << 8)
                            | (dstCntxt.getAddressPart(1) & 0xFF),
                            dstCntxt.getAddressPart(2),
                            dstCntxt.getAddressPart(3),
                            dstCntxt.getAddressPart(4),
                            dstCntxt.getAddressPart(5),
                    c.getUint16_t(),
                    d.getUint16_t());
            break;
        }
        }
    }
    return true;
}

bool IPHCDecompressor::getNHCEncodedExtensionHeader(
        uint8_t nhcDispatch, cometos::Airframe& frame,
        BufferInformation* buf, FirstFragBufInfo & ffbi,
        bool& nextCompressed)
{
    FollowingHeader* eh = NULL;
    uint8_t len = 0;
    FollowingHeader* addHeader = NULL;
    bool anotherIP = false;

    // next header inline means the next byte after the NHC dispatch byte
    // will be the next header field
    if (isNHInlineNHC(nhcDispatch)) {

        // retrieve next header field, if it is not elided
        uint8_t nhType;
        frame >> nhType;

        // if compression stops here, an additional next header byte is added
        // (additionally to NHC field and len), which is
        // counted separately (rest of extension header is counted later)
        ffbi.compressedSize += sizeof(nhType);

        // add next header and set to uncompressed
        // (next header inline means no more NHC)
        LOG_DEBUG("here");
        addHeader = IPv6Datagram::createNextHeader(nhType);
        nextUncompressed = addHeader;
        if (addHeader == NULL) return 0;
        nextCompressed = false;
    }

    // determine type of this extension header and retrieve accompanying data
    switch(getEHId(nhcDispatch)) {
        case EH_EID_HBH: {
            eh = new IPv6HopByHopOptionsHeader();
            frame >> len;
            ffbi.compressedSize += sizeof(len);
            break;}
        case EH_EID_ROUTING: {
            eh = new IPv6RoutingHeader();
            IPv6RoutingHeader* rh = (IPv6RoutingHeader*)eh;
            frame >> len;
            len -= 2;
            uint8_t a, b;
            frame >> a >> b;
            rh->setRoutingType(a);
            rh->setSegmentsLeft(b);
            ffbi.compressedSize += sizeof(len) + sizeof(a) + sizeof(b);
            break;}
        case EH_EID_FRGMNT: {
            eh = new IPv6FragmentHeader();
            IPv6FragmentHeader* fh = (IPv6FragmentHeader*)eh;
            uint8_t a, b;
            uint32_nbo c;
            frame >> a >> b >> c;
            fh->setFragmentOffset((a << 5) | b >> 3);
            if((b & 1) == 1) fh->setMFlag();
            fh->setIdentification(c.getUint32_t());
            ffbi.compressedSize += sizeof(a) + sizeof(b) + sizeof(c);
            break;}
        case EH_EID_DSTOBT: {
            eh = new IPv6DestinationOptionsHeader();
            frame >> len;
            ffbi.compressedSize += len;
            break;}
        case EH_EID_IPv6: {
            //TODO: How to handle encapsulated IPv6 Packets
            ASSERT(false);
            anotherIP = true;
            break;}
        default: {
            ASSERT(false);
            return false;
        }
    }

    // any remaining data in the extension header is not stored in the
    // corresponding objects, but as a pointer to a data buffer;
    // retrieve the extension header data and store it into the given buffer
    if (!anotherIP && eh != NULL) {
        if (len > 0) {
            if (len > frame.getLength()) {
                // a compressed extension header MUST NOT overlap into the
                // next fragment!
                ASSERT(false);
                return false;
            }

            // write extension header data to buffer at offset of last
            // extension header's data
            eh->setData((buf->getContent() + ffbi.numWrittenToBuf), len);
            ffbi.compressedSize += len;
            while (len > 0) {
                frame >> (*buf)[ffbi.numWrittenToBuf++];
                len--;
            }
        }
        datagram->addHeader(eh);
        ASSERT(false);
    } else {
        ASSERT(false);
        return false;
    }

    // in uncompressed logic headers have to be multiples of 8 octets
    uint8_t uncomprSize = eh->getAlignedSize();
    ffbi.uncompressedSize += uncomprSize;

    // if there was an uncompressed header, add it to the datagram
//    if (addHeader != NULL) {
//        datagram->addHeader(addHeader);
//    }

    LOG_DEBUG("Decompressed extHeader of type "
               << eh->getHeaderType()
               << "; read "
               << (int) ffbi.compressedSize << " bytes from frame; remaining len: "
               << (int) frame.getLength());
    return true;
}

bool IPHCDecompressor::getNHCEncodedUDPHeader(
        uint8_t nHead, cometos::Airframe& frame,
        BufferInformation* buf, FirstFragBufInfo& ffbi)
{
    UDPPacket *packet = new UDPPacket();
    ASSERT(packet != NULL);
    switch(getUDPPortCompression(nHead)) {
        case UDP_INLINE: {
            uint16_nbo a, b;
            frame >> a >> b;
            ffbi.compressedSize += sizeof(a) + sizeof(b);
            packet->setSrcPort(a.getUint16_t());
            packet->setDestPort(b.getUint16_t());
            break;
        }
        case UDP_SRC_INLINE: {
            uint16_nbo a;
            uint8_t b;
            frame >> a >> b;
            ffbi.compressedSize += sizeof(a) + sizeof(b);
            packet->setSrcPort(a.getUint16_t());
            packet->setDestPort(0xF000 | b);
            break;
        }
        case UDP_DST_INLINE: {
            uint8_t a;
            uint16_nbo b;
            frame >> a >> b;
            ffbi.compressedSize += sizeof(a) + sizeof(b);
            packet->setSrcPort(0xF000 | a);
            packet->setDestPort(b.getUint16_t());
            break;
        }
        case UDP_SHORT_PORTS: {
            uint8_t a;
            frame >> a;
            ffbi.compressedSize += sizeof(a);
            packet->setSrcPort(0xF0B0 | (a >> 4));
            packet->setDestPort(0xF0B0 | (a & 0xF));
            break;
        }
    }
    if (udpHasChecksum(nHead)) {
        uint16_nbo a;
        frame >> a;
        ffbi.compressedSize += sizeof(a);
        packet->setChecksum(a.getUint16_t());
    }

    LOG_DEBUG("Decompr. UDP header; read " << (int) ffbi.compressedSize
                << " bytes from frame; remaining len: " << (int) frame.getLength());

    uint8_t rest = frame.getLength();
    packet->setData((buf->getContent() + ffbi.numWrittenToBuf),
            rest);
    if ((buf->getSize() - (ffbi.numWrittenToBuf + rest)) > 0) {
        buf->freeEnd((uint16_t)(buf->getSize() - (ffbi.numWrittenToBuf + rest)));
    }

    ffbi.compressedSize += rest;
    ffbi.uncompressedSize += UDPPacket::UDP_HEADER_SIZE + rest;
    while (frame.getLength() > 0) {
        frame >> (*buf)[ffbi.numWrittenToBuf++];
    }

    datagram->addHeader(packet);

    LOG_DEBUG("Got payload; read " << (int) ffbi.compressedSize
            << " bytes from frame; remaining len: " << (int) frame.getLength());

    return true;
}

} /* namespace cometos_v6 */
