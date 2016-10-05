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

#ifndef LFFRPACKET_H_
#define LFFRPACKET_H_

#include "RetransmissionList.h"
#include "IPHCCompressor.h"
#include "QueueObject.h"
#include "LFFRObject.h"

namespace cometos_v6 {
const uint32_t SEND_ALL_SEQUENCE_NUMBERS = 0xFFFFFFFF;
class LFFRPacket : public QueueObject, public LFFRObject {

private:
    const uint8_t INVALID_VALUE;
    DatagramInformation* _datagramInformation;
    bool _isFragmented;
    uint32_t _transmissionList;

public:
    LFFRPacket(DatagramInformation* datagramInformation,
               uint32_t transmissionList = SEND_ALL_SEQUENCE_NUMBERS);
    /* Takes In: empty airframe and airframe size
     * Returns: LFFR Fragment with size less than maxSize or IP datagram
     * with compressed header inside frame.
     * Purpose: Create the next valid LFFR fragment and return it inside
     * the input airframe, in case the IP datagram represented by this
     * LFFRPackect is large and needs to be Fragmented. If the IP datagram
     * is small enough to fit inside a MAC frame (less than maxSize == 81),
     * then its header is compressed and the datagram is returned inside frame
     * */
    void createFrame(cometos::Airframe& frame,
                     uint8_t maxSize,
                     LowpanFragMetadata& fragHead,
                     const IPv6Datagram* & dg);
    QueueObject::response_t response(bool success,
                                     const cometos::MacTxInfo& info);
    const Ieee802154MacAddress& getDstMAC() const;

    uint8_t getPRCValue() const;
    uint8_t getSRCValue() const;
    uint8_t getHRCValue() const;
    inline void logEnqueue() const { LOG_DEBUG("LFFR Packet in Queue"); }

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const;

    virtual bool representsDatagram() const {
        return true;
    }

    virtual LocalDgId getDgId(bool& valid) const;

    virtual const IPv6Datagram* getDg() const;

    virtual uint8_t currOffset() const;

    virtual uint16_t getCurrDgSize() const;

    bool canBeDeleted() const;
    virtual ~LFFRPacket();

private:
    /* Takes In: A frame to be poulated with data and LFFR headers.
     * maxSize is the size of the frame and sequenceNumber is a valid
     * LFFR sequence number for the datagram fragment.
     *
     * Purpose: Gives back a ready to send
     * LFFR frame with all proper headers attached. LFFR headers are not
     * attached to the returned frame if the datagram is contained with-
     * in 81 bytes (maxSize) and hence does not need fragmentation. The
     * function also indicates if the returned fragment/frame is the
     * last one by returning the isLastFrame value to true.
     */
    bool createFragment(cometos::Airframe& frame,
                        uint8_t maxSize,
                        uint8_t sequenceNumber,
                        bool& isLastFrame,
                        LowpanFragMetadata& fragMeta,
                        const IPv6Datagram* & dg);

    /* purpose: It attaches the compressed header to
     * the first fragment (the one indicated by sequence number 0).
     * It also returns the offset value (that is calculated considering
     * the uncompressed header size). This value is used to populate the
     * Offset field in the LFFR Header. The function sets the value of
     * isLastFrame to true if the current frame is the
     * last will be the last one for this datagram
     *
     * Returns an empty frame and isLastFrame == true, if sequenceNumber
     * out of bounds
     */
    uint8_t populateDataGramIntoFrame(cometos::Airframe& frame,
                                      uint8_t sequenceNumber, uint8_t maxSize,
                                      bool& isLastFrame, bool& isFragmented);
    void addHeaderDataToFrame(cometos::Airframe& frame,
                              uint8_t* compressedHeader,
                              uint8_t sizeOfCompressedHeader);
    // void createHeader(//uint8_t)

    /*
     * The function reads in the _transmissionList member and
     * returns the first sequence number inside _transmissionList
     * that is set for a transmission/retransmission
     * */
    uint8_t getNextSeqNumForTransmission();
    inline bool isSeqNumInValidRange(uint8_t seqNumber) {
        return (seqNumber >= 0 && seqNumber < 32);
    }
    inline void clearTransmissionlist() {
        _transmissionList = 0x00000000;
    };
    void clearSeqNumFrmTransmissionList(uint8_t sequenceNumber);
    uint8_t fetchDataForFirstFrame(cometos::Airframe& frame, uint8_t maxSize,
                                   bool& isLastFrame, bool& isFragmented);
    uint8_t fetchDataForSubsequentFrame(cometos::Airframe& frame,
                                        uint8_t sequenceNumber, uint8_t maxSize,
                                        bool& isLastFrame);

    uint16_t getCompDgramSize(uint8_t sizeofCompressedHeaders,
                              FollowingHeader* firstUnCompressedHeader);
    uint16_t getSizeOfUncompressedPart(FollowingHeader* unCompressedNextHeader);

    uint16_t getsizeOfUncompressedHeaders(
        FollowingHeader* firstUnCompressedHeader);
    void updateDatagramCompressionInfo(uint8_t sizeofCompressedHeaders,
                                       FollowingHeader* firstUnCompressedHeader,
                                       uint8_t sizeOfFirstFragment,
                                       uint16_t sizeOfUnCompressedHeader);
    uint16_t putUncompressedHeaderDataInBuffer(
        uint8_t* buffer, uint8_t bufferSize,
        FollowingHeader* unCompressedHeader, uint16_t startPosInHeader = 0);
    FollowingHeader* calculateHeaderAndPosInHdrFromSeqNumber(
        FollowingHeader* firstUnCompressedHeader, uint8_t sequenceNumber,
        uint16_t& positionInHeader);

    FollowingHeader* getRelevantUncompressedHeader(
        uint16_t UncompressedBytesAlreadyTransmitted,
        uint16_t& positionInReturnedHeader);
    bool isLastFragmentToTransmit(uint8_t sequenceNumber);

    uint8_t highestOffset;
};
} /* namespace cometos_v6 */

#endif /* LFFRPACKET_H_ */
