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

#ifndef IPHC_COMPRESSOR_H_
#define IPHC_COMPRESSOR_H_

#include "cometos.h"
#include "Airframe.h"
#include "IPv6Datagram.h"
#include "Ieee802154MacAddress.h"
#include "UDPPacket.h"
#include "LowpanBuffer.h"
#include "IPv6Request.h"

#define SIZE_OF_LOWPAN_SUBSEQUENT_HEADERS 5
#define LOWPAN_FIRSTFRAGMENT_HEADER_SIZE 4
namespace cometos_v6 {

class IPHCCompressor {
public:
    IPHCCompressor(IPv6Datagram* datagram,
                   const Ieee802154MacAddress& srcMAC,
                   const Ieee802154MacAddress& dstMAC,
                   uint8_t typeOfUncontainedNextHeader);


    IPHCCompressor(IPv6Request* req,
                   uint8_t typeOfUncontainedNextHeader = FollowingHeader::NoNextHeaderNumber);

    ~IPHCCompressor();

    /**
     * Stores the datagram given to this object via the constructor in an
     * airframe, up to a maximum of maxSize bytes. Headers contained within
     * the the datagram will be compressed according to LOWPAN_IPHC and
     * LOWPAN_NHC, if applicable.
     *
     * @param[in,out] frame   airframe to store the datagram's headers/content to
     * @param[in]     maxSize maximum number of bytes to put into the airframe
     * @param[in,out] buf     pointer to the buffer that stores the actual
     *                        payload of the datagram; bytes that are written to
     *                        the airframe will be freed by this method
     * @param[in,out] posInBuf index of the current position of the buf object
     *                         pointing to the next byte to be send/written
     * @param[in]     onlyIP  flag; if set, only the IPv6 Header itself and the
     *                        last header will be compressed
     * @return size of the data written to the airframe in its uncompressed form
     */
    uint16_t streamDatagramToFrame(cometos::Airframe& frame,
                              uint8_t maxSize,
                              BufferInformation* buf,
                              uint16_t& posInBuf
//                              , bool onlyIP = false
                              );


    /**
     * Returns whether this object's datagram has been completely written
     * into some buffer.
     *
     * @return true, if all headers and data have been put to a buffer using compressDatagram
     *               and/or putUncompressedDataInBuffer
     *         false, else
     */
    bool isDone();

    const Ieee802154MacAddress& getsrcMAC() const { return srcMAC; }

    const IPv6Datagram* getDatagram() const { return datagram; }
//    void putUncompressedDataInBuffer(uint8_t* buffer,
//                                     FollowingHeader* firstUnCompressedHeader,
//                                     uint8_t blockSize,
//                                     uint16_t positionInHeader = 0);

    uint8_t putCompressibleHeadersInBuffer(
                uint8_t* buffer,
                uint8_t bufferSize,
                FollowingHeader*& unCompressedNextHeader,
                uint8_t spaceForLowpanHeader = LOWPAN_FIRSTFRAGMENT_HEADER_SIZE
//                , bool onlyIP = false
                );


    inline uint16_t getUncompHeaderSize(){
        return (uncomprSize);
    }


private:

    static const uint8_t LOWPAN_NHC_EID_HOP_BY_HOP_OPTION = 0xE0;
    static const uint8_t LOWPAN_NHC_EID_ROUTING = 0xE2;
    static const uint8_t LOWPAN_NHC_EID_FRAGMENTATION = 0xE4;
    static const uint8_t LOWPAN_NHC_EID_DESTINATION_OPTION = 0xE6;
    static const uint8_t LOWPAN_NHC_UDP_DISPATCH = 0xF0;

    /**
     * Compresses traffic class and flow label and sets LOWPAN_IPHC header
     * fields accordingly.
     *
     * @param[in,out] buf ptr to first byte of LOWPAN_IPHC encoding
     * @param[in,out] pos offset to current writing position in LOWPAN_IPHC-
     *                    encoded datagram; will be incremented for every
     *                    inlined byte written to buf, so that it again
     *                    points to the next byte to be written
     */
    void comprTCFL(uint8_t* buf, uint8_t& pos);

    /**
     * Compresses hop limit and sets LOWPAN_IPHC header fields accordingly.
     *
     * @param[in,out] buf ptr to first byte of LOWPAN_IPHC encoding
     * @param[in,out] pos offset to current writing position in LOWPAN_IPHC-
     *                    encoded datagram; will be incremented for every
     *                    inlined byte written to buf; points to pos next
     *                    to be written afterwards
     */
    void comprHopLimit(uint8_t* buf, uint8_t& pos);

    /**
     * Compresses source address and sets LOWPAN_IPHC header fields accordingly.
     *
     * @param[in,out] buf ptr to first byte of LOWPAN_IPHC encoding
     * @param[in,out] pos offset to current writing position in LOWPAN_IPHC-
     *                    encoded datagram; will be incremented for every
     *                    inlined byte written to buf; points to pos next
     *                    to be written afterwards
     */
    void comprSrcAddr(uint8_t* buf, uint8_t& pos);

    /**
     * Compresses destination address and sets LOWPAN_IPHC header fields accordingly.
     *
     * @param[in,out] buf ptr to first byte of LOWPAN_IPHC encoding
     * @param[in,out] pos offset to current writing position in LOWPAN_IPHC-
     *                    encoded datagram; will be incremented for every
     *                    inlined byte written to buf; points to pos next
     *                    to be written afterwards
     */
    void comprDstAddr(uint8_t* buf, uint8_t& pos);


    void storeIPAddress(const IPv6Address& address, uint8_t start, uint8_t* buf,
                        uint8_t& pos);

    /*
     *
     * * @param[out] alsoCompressNextHeader
     *     set by the method, if the next header can also be compressed,
     *     cleared if not
     */
    uint16_t comprNextHeader(uint8_t* buf,
                             uint8_t& pos,
                             uint8_t maxSize,
                             bool& alsoCompressNextHeader,
                             uint8_t spaceForLowpanHeader);

    /**
     * Sets NHC NH field depending on its type and the space left in buffer.
     * If next header field has to be inlined, it is additionally written
     *
     * @param[in] next pointer to the next header
     * @param[in] nhcDispatch
     *     NHC encoding for the current header, not including bit 0 (NH)
     * @param[out] buf
     *     pointer to data buffer to which the NHC encoding should be stored
     * @param[in, out] pos
     *     index into buf, marking the position at which to write the NHC byte
     * @param[in] maxSize
     *     maximum size of the frame
     * @param[in] spaceForLowpanHeader
     *     byte length of a possible additional header
     * @return true, if next header in header list can also be compressed
     *         false, if not
     *
     */
    bool writeNHCNextheader(const FollowingHeader* next,
                                  uint8_t nhcDispatch,
                                  uint8_t* buf,
                                  uint8_t& pos,
                                  uint8_t maxSize,
                                  uint8_t spaceForLowpanHeader);

    void setUDPHeader(UDPPacket* udpPacket, uint8_t* buf, uint8_t& pos,
                      uint8_t maxSize);

    uint8_t diff(uint8_t minuend, uint8_t subtrahent);

    IPv6Datagram* datagram;
    Ieee802154MacAddress srcMAC;
    Ieee802154MacAddress dstMAC;
    FollowingHeader* nHeader;
    uint16_t uncomprSize;
    uint16_t posInCurrUncompressedHeader;
    uint8_t typeOfUncontainedNextHeader;


};

} /* namespace cometos_v6 */

#endif /* IPHCCompressorM_H_ */
