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

#ifndef DATAGRAM_REASSEMBLY_H_
#define DATAGRAM_REASSEMBLY_H_

#include "LowpanBuffer.h"
#include "Ieee802154MacAddress.h"
#include "IPHCDecompressor.h"
#include "LinkLayerInformation.h"

namespace cometos_v6 {

#if defined SWIG || defined BOARD_python
enum assemblyStatus_t {
#else
enum assemblyStatus_t : uint8_t {
#endif
    ASSEMBLYBUFFER_STATUS_FREE            = 0,
    ASSEMBLYBUFFER_STATUS_TIMEOUT_PENDING = 1,
    ASSEMBLYBUFFER_STATUS_RECEIVED        = 2,
    ASSEMBLYBUFFER_STATUS_DONE            = 3,
    ASSEMBLYBUFFER_STATUS_ERROR           = 0xFF,
};

/**
 * Structure to keep track of a fragmented IPv6 datagram in the
 * process of reassembling it.
 */
class DatagramReassembly {
public:
//    template <uint8_t MEntries> friend class AssemblyBuffer;

    DatagramReassembly():
        uncompressedBufferPos(0), tag(0), size(0),
        ticksLeft(ASSEMBLYBUFFER_STATUS_FREE), buffer(NULL), dcDatagram(NULL), acknowledgementBitmap(0) {};

    ~DatagramReassembly() {
        this->free();
    }

    void initialize(const Ieee802154MacAddress& srcMac,
                    BufferInformation* buf,
                    uint16_t tag,
                    uint16_t size);

    void free();

    uint16_t getContiguousSize() const;

    inline bool isDone() {
        return (ticksLeft == ASSEMBLYBUFFER_STATUS_DONE);
    }

    /** Adds a fragment contained in the given airframe to the
     * reassembly process.
     *
     * @param lowpanFragmentOffset
     *          offset value as given in the LOWPAN_FRAGX header or
     *          implicitly 0 in the FRAG1 header
     * @param frame
     *          airframe containing the actual fragment data
     * @param uncompressedSize
     *          uncompressed size of the fragment to be added; must be
     *          a multiple of LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT
     * @return  true, if fragment was added successfully or is a duplicate
     *                and therefore already present
     *          false, if fragments are overlapping, according to the given
     *                 offset
     */
    bool addFragment(uint8_t lowpanFragmentOffset,
                     cometos::Airframe& frame,
                     uint16_t uncompressedSize);

    /** Updates the link layer quality info associated with the reassembled
     * datagram by adding the information contained within the given frame.
     * @param[in] frame frame containing link layer information
     */
    void updateLlRxInfo(const cometos::Airframe & frame);

    /** Get the buffer associated with the reassembly process.
     * Ownership remains with the FragmentInformation object.
     * @return pointer to the buffer information object associated with
     *         the reassembly process
     */
    BufferInformation* getBuffer() {
        return buffer;
    }

    void tick() {
        ticksLeft--;
    }

    /* Take ownership of the associated buffer.
     * @return pointer to the buffer information objects associated with
     *         the reassembly process.
     */
    BufferInformation* decapsulateBuffer() {
        BufferInformation* ret = buffer;
        buffer = NULL;
        return ret;
    }

    IPv6Datagram* getDatagram();

    bool equals(DatagramReassembly const & other)
    {
        return this->MACAddr == other.MACAddr
                && this->tag == other.tag
                && this->size == other.size;
    }

    bool belongsToDatagram(Ieee802154MacAddress const & macAddr,
                           uint16_t tag,
                           uint16_t size)
    {
        return this->MACAddr == macAddr
            && this->tag == tag
            && this->size == size;
    }

    const LlRxInfo & getLlRxInfo() {
        return rxInfo;
    }

    IPHCDecompressor* getDcDatagram() const {
        return dcDatagram;
    }

    void setDecompressor(IPHCDecompressor* dc) {
        dcDatagram = dc;
    }

    /* Initialize FragmentInformation with data from decompression.
     * @param startUncompressedHeaders
     *     index in buffer where the first, but potentially incomplete
     *     uncompressed header starts
     * @param writeIndex
     *     index in buffer
     *
     */
    void setDecompressionInfo(uint16_t uncompressedBufferPos,
                              uint8_t uncompressedOffset);

    const Ieee802154MacAddress& getMACAddr() const {
        return MACAddr;
    }

    uint16_t getTag() const {
        return tag;
    }

    uint8_t getStatus() {
        return ticksLeft;
    }

    // The method accepts a valid 5 bit sequence number 'n'[0-31]
    // and sets the 'n+1'th Most Significant Bit inside the 32 bit
    // AcknowledgmentBitmap. Intended as a way to track
    // sequence numbers that were received. Ignores sequence
    // numbers outside valid range [0 31]
    inline void storeSequenceNumberInAckBitmap(uint8_t seqNum){
        acknowledgementBitmap |= 0x80000000 >> seqNum;
    }

    inline uint32_t getAckBitmap(){
        return acknowledgementBitmap;
    }

private:
    static const uint8_t OCCUPIED_ARRAY_SIZE = 16;

    /** Clear bits in occupied bitmap */
    void clearOccupied();

    /** Clear bits in ack bitmap */
    void clearAckBitMap();

    /** Set value combi of array index and bitmask to the next bit.
     * E.g. for Bit 15 (index=0, mask=0x00)
     *
     * @param[in,out] bitmapArrayindex
     *    Current position in array, will be updated to next position in array
     * @param[in,out] bitmapBitMask
     *    Bit mask for current array index, changed to point to next bit
     */
    void nextBitInBitMap(
            uint8_t & bitmapArrayIndex,
            uint16_t & bitmapBitMask);

    /** 32 byte bitmap, marking received octets of the datagram*/
    uint16_t                    occupied[OCCUPIED_ARRAY_SIZE];

    /** Position in buffer where the first byte of the first
     *  uncompressed data (header or payload) is stored; used for parsing
     *  the headers as soon as the datagram is reassembled completely and
     *  for computing the storage position of incoming fragments */
    uint16_t                    uncompressedBufferPos;

    /** stores offset corresponding to the uncompressedBufferPos;
     * combination of the two can be used to determine the position
     * in the buffer to store incoming fragments according to their offset
     */
    uint8_t                     uncompressedOffset;

    /** number of bytes present after the 8-octet represented by the last
     * occupied bit. Needed to correctly compute the size of a datagram after
     * the last fragment -- which does not need to be a multiple of 8 octets in
     * size -- has been added to the buffer */
//    uint8_t                     numAdditionalOctets;

    /** LOWPAN Tag associated with this reassembly process */
    uint16_t                    tag;

    /** Uncompressed size of the whole datagram in bytes */
    uint16_t                    size;

    /** MAC source address associated with this datagram */
    Ieee802154MacAddress        MACAddr;

    /** status of the datagram, also used to count timeout ticks */
    uint8_t                     ticksLeft;

    /** associated reassembly buffer */
    BufferInformation*          buffer;

    /** associated datagram with decompression functionality */
    IPHCDecompressor*  dcDatagram;

    /** associated link layer reception quality information */
    LlRxInfo                    rxInfo;

    /** LFFR-related NACK bitmap */
    uint32_t                    acknowledgementBitmap;
};

} // namespace

#endif /* REASSEMBLYINFORMATION_H_ */
