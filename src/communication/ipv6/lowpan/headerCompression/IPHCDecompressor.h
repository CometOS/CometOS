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

#ifndef IPHC_DECOMPRESSOR_H_
#define IPHC_DECOMPRESSOR_H_

#include "IPv6Datagram.h"
#include "LowpanBuffer.h"
#include "Ieee802154MacAddress.h"
#include "FollowingHeader.h"
#include "cometos.h"

namespace cometos_v6 {

struct contextNrs_t {
    uint8_t src: 4;
    uint8_t dst: 4;
    contextNrs_t(): src(0), dst(0) {}
};

struct FirstFragBufInfo {
    uint16_t    compressedSize;
    uint16_t    uncompressedSize;
    uint16_t    numWrittenToBuf;
    FirstFragBufInfo(): compressedSize(0), uncompressedSize(0), numWrittenToBuf(0) {}
};

/**
 * Implements the decompression logic of LOWPAN_IPHC and LOWPAN_NHC
 * of RFC 6282. Usage involves three steps:
 * <ol>
 * <li>Create: Parses the IPv6 Header from an uncompressed or
 *             LOWPAN_IPHC-encoded datagram and stores the information within
 *             an IPv6Datagram object, this does not require an additional
 *             buffer.</li>
 * <li>nextCompressed: If LOWPAN_IPHC was used, decompresses the next
 *                     LOWPAN_NHC headers and appends them to the IPv6Datagram
 *                     object. A buffer is needed to store additional optional
 *                     header information which does not fit into the header
 *                     structures itself. This method CAN ALWAYS called on
 *                     the first fragment of a fragmented datagram, because
 *                     compression is only allowed for headers that fit in the
 *                     first fragment</li>
 * <li>nextUncompressed:
 *       This will parse any remaining uncompressed headers and append the
 *       contained information to the IPv6Datagram next header's list.
 *       These have to be stored in a buffer before. Therefore, this method
 *       should be called after reassembly of the datagram is complete.
 * </ol>
 *
 * The handling is a bit inconsistent, i.e., some parts of compressed headers
 * are written to a buffer, uncompressed headers are always written to a buffer
 * and the IPv6 header itself is never written to a buffer.
 *
 * The different steps are needed because we first have to get information
 * about the buffer space (possibly needing the IPv6 header first to match
 * any reassembly tag) to extract the compressed headers and because the
 * uncompressed headers may strech over several fragments so that it DOES
 * make sense to parse them only after reassembly, when they are guaranteed
 * to be complete.
 */
class IPHCDecompressor {
public:
    static const uint8_t IPV6_HEADER_SIZE_UNCOMPRESSED = 40;
    static const uint8_t LOWPAN_IPHC_MINIMUM_HEADER_SIZE = 2;
    static const uint8_t IPV6_COMPRESSED_HEADER_SIZE_INVALID = 0xFF;

    /** Retrieves an IPHC or uncompressed IPv6 header from the given Airframe.
     * and stores the contained information in a newly created IPv6Datagram.
     * If the header is invalid or of an unknown format,
     * the contained datagram remains NULL.
     */
    IPHCDecompressor(uint8_t lowpanHeader,
            cometos::Airframe& frame,
            const Ieee802154MacAddress& srcMAC,
            const Ieee802154MacAddress& dstMAC,
            const IPv6Context* srcCntxt,
            const IPv6Context* dstCntxt);


    ~IPHCDecompressor();

    /**
     * If there are any compressed headers corresponding to the LOWPAN_NHC
     * format, they are decompressed and their content is stored in the
     * buffer passed to the method.
     *
     * @param frame Airframe to check for next headers. If compressed next
     *              headers are found, they are deserialized from the frame
     * @param buf   location information to a buffer to store the decompressed
     *              information from the extension headers;
     * @return struct containing the compressed and uncompressed sizes
     */
    FirstFragBufInfo compressedNext(cometos::Airframe& frame, BufferInformation* buf);

    /**
     * This method parses the remaining uncompressed next headers of an
     * IPv6Datagram, until the end of the chain of headers is reached.
     * When returning true, this object's datagram will contain all parsed
     * headers in the order in which they appeared.
     *
     * @param buf buffer information object, from which to parse the headers
     * @param bufferPos position from where to start within the buf
     * @return true, if header were parsed successfully, false if invalid data
     *         was encountered
     */
    bool uncompressedNext(BufferInformation* buf, uint16_t& bufferPos);

    uint8_t getNextUncompressedHeaderType();

    /** Removes this object's IPv6Datagram object. Ownership  and
     * responsibility to delete the object pass to the
     * caller of this function.
     * @return pointer to the dynamically allocated datagram object, containing
     *         all information added to it so far
     */
    IPv6Datagram* decapsulateIPDatagram() {
        IPv6Datagram* p = datagram;
        datagram = NULL;
        return p;
    }

    /** Returns a read-only pointer to this object's IPv6Datagram object.
     * Ownership does NOT pass to the caller of this method.
     * @return const pointer to the datagram
     */
    const IPv6Datagram * const getIPDatagram() const {
        return datagram;
    }

    /** Get a read-only pointer to the destination address of this
     * object's datagram
     * @return const pointer to the IPv6 dest address
     */
    const IPv6Address* getIPv6Dest() const {
        if (datagram != NULL) {
            return &(datagram->dst);
        }
        return NULL;
    }

    /** Get the hop limit of this object's datagram
     * @return hop limit
     */
    uint8_t getHopLimit() const {
        if (datagram != NULL) {
            return datagram->getHopLimit();
        }
        return 0;
    }

private:

    /**
     * Expects the "next" bytes in Airframe to be in LOWPAN_IPHC format
     * and stores the contained information in this objects' IPv6Datagram.
     * If successful, the objects' IPv6Datagram will be filled with valid
     * information.
     *
     * @param iphcByte1  first (dispatch) byte of LOWPAN_IPHC encoding
     * @param frame      airframe containing the data
     * @param srcMAC     IEEE 802.15.4 address of linklayer origin of the frame
     * @param dstMAC     IEEE 802.15.4 address of linklayer dest. of the frame
     * @param srcCntxt   source IPv6Contexts of this node; used for
     *                   decompression depending on IPHC header
     * @param dstCntxt   dest IPv6Contexts of this node;
     * @return true, if IPv6Header was decompressed successfully
     *         false, if invalid information was found in the frame
     *
     */
    bool decompressIPHeader(uint8_t iphcByte1,
            cometos::Airframe& frame,
            const Ieee802154MacAddress& srcMAC,
            const Ieee802154MacAddress& dstMAC,
            const IPv6Context* srcCntxt,
            const IPv6Context* dstCntxt);

    /**
     * Reads the CID bit from the IPHC second header byte and retrieves
     * any context information (3rd IPHC header byte) if present.
     * @param[in]     iphcByte2 second IPHC header byte
     * @param[in,out] frame     airframe containing the IPHC-compressed IPv6 header
     * @param[out]    cnr       any retrieved context information is stored there
     * @param[out]    bytesReadFromFrame number of bytes removed from the frame
     * @return true, if context info was read successfully, false if frame invalid
     */
    bool getContext(uint8_t iphcByte2, cometos::Airframe& frame, contextNrs_t& cnr, uint8_t & bytesReadFromFrame);

    /**
     * Checks the value of the TF field of LOWPAN_IPHC and -- depending
     * on its value -- retrieves the inlined traffic class and flow label field
     * from the given airframe. The read values are stored in this object's
     * datagram object.
     *
     * @param head iphc header byte 1
     * @param frame airframe containing the rest of the iphc-encoded header
     * @param[out]    bytesReadFromFrame number of bytes removed from the frame
     * @return true, if valid content was retrieved, false if frame is invalid
     */
    bool getTrafficClassFlowLabel(uint8_t head, cometos::Airframe& frame, uint8_t & bytesReadFromFrame);


    /**
     * Checks the hop-limit field of the IPHC header an retrieves the inlined
     * hop limit field if it exists. The determined value is stored in this
     * object's datagram object.
     *
     * @param head  iphc header byte 1
     * @param frame airframe containing the rest of the iphc-compressed header
     * @param[out]    bytesReadFromFrame number of bytes removed from the frame
     * @return true, if valid content was retrieved, false if frame is invalid
     */
    bool getHopLimit(uint8_t head, cometos::Airframe& frame, uint8_t & bytesReadFromFrame);

    /**
     * Retrieves an IPv6 address from an airframe.
     * @param[in] contextMode indicates if stateful (context-based) or stateless
     *                        compression should be used
     * @param[in] addressMode indicates stateless addressing mode
     * @param[in] MACAddr     IEEE 802.15.4 address corresponding to IPv6 addr
     * @param[in,out] frame   airframe containing the inlined/compressed address
     * @param[in] cntxt       reference to context to be used for decompression
     * @param[out]    bytesReadFromFrame number of bytes removed from the frame
     * @return IPv6 address object read from the frame/calculated from the context
     */
    IPv6Address getIPAddress(bool contextMode, uint8_t addressMode,
            const Ieee802154MacAddress& MACAddr, cometos::Airframe& frame,
            const IPv6Context& cntxt, uint8_t & bytesReadFromFrame);


    /**
     * Check compression mode of destination address and retrieve the
     * compressed/inlined fields from the frame. The retrieved address
     * is stored in this object's datagram object.
     *
     * @param[in] iphcByte2 second byte of iphc header
     * @param[in] MACAddr   corresponding IEEE 802.15.4 mac addr to the IPv6 src addr
     * @param[in,out] frame     airframe containing the inlined/compressed fields
     * @param[in] srcCntxt  destination context for this frame
     * @param[out]    bytesReadFromFrame number of bytes removed from the frame
     * @return true, if valid frame was decoded, false if frame is invalid
     */
    bool getSrcAddress(uint8_t iphcByte2,
            const Ieee802154MacAddress& MACAddr,
            cometos::Airframe& frame,
            const IPv6Context& srcCntxt, uint8_t & bytesReadFromFrame);

    /**
     * Check compression mode of destination address and retrieve the
     * compressed or inlined fields from the frame. Retrieved address
     * is stored in this object's datagram object.
     *
     * @param[in] iphcByte2 second byte of iphc header
     * @param[in] MACAddr   corresponding IEEE 802.15.4 mac addr to the IPv6 dest addr
     * @param[in,out] frame     airframe containing the inlined/compressed fields
     * @param[in] dstCntxt  destination context for this frame
     * @param[out]    bytesReadFromFrame number of bytes removed from the frame
     * @return true, if valid frame was decoded, false if frame is invalid
     */
    bool getDstAddress(uint8_t head2,
            const Ieee802154MacAddress& MACAddr,
            cometos::Airframe& frame,
            const IPv6Context& dstCntxt, uint8_t & bytesReadFromFrame);

    /**
     * Retrieves an extension header from the given Airframe, puts its
     * data into the given buffer and updates the buffer position accordingly.
     * Also indicates if the next header is compressed according to LOWPAN_NHC
     * or uncompressed (i.e., the next header byte is carried inline)
     *
     * @param[in]     nhcDispatch first byte of LOWPAN_NHC encoding
     * @param[in,out] frame airframe containing the extension header
     * @param[in,out] buf   managed buffer pointer
     * @param[in,out] ffbi  information about compressed/uncompressed size of
     *                      header up to this point. This header's sizes and
     *                      number of bytes written to buffer are added to it
     * return true, if next NHC-encoded header was retrieved from frame
     *        false, if some error occurred (e.g. len did not match frame size)
     *
     */
    bool getNHCEncodedExtensionHeader(uint8_t nhcDispatch, cometos::Airframe& frame,
            BufferInformation* buf, FirstFragBufInfo& ffbi, bool& nextCompressed);

    /**
     *
     */
    bool getNHCEncodedUDPHeader(uint8_t nHead, cometos::Airframe& frame,
            BufferInformation* buf, FirstFragBufInfo& ffbi);

    IPv6Datagram*       datagram;  //< this object's IPv6Datagram

    /** pointer to next uncompressed header encountered in the frame
     * used to indicate presence of remaining uncompressed headers */
    FollowingHeader*    nextUncompressed;

    uint8_t compressedIPv6HeaderSize;
};

} /* namespace cometos_v6 */
#endif /* IPHCDecompressorM_H_ */
