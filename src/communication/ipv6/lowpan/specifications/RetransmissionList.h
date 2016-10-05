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

#ifndef RETRANSMISSIONLIST_H_
#define RETRANSMISSIONLIST_H_

#include "LowpanBuffer.h"
#include "IPv6Request.h"
#include "AssemblyBuffer.h"

namespace cometos_v6 {
class LowpanAdaptionLayer;
const uint32_t RTO_INITIAL_VALUE = 1028;
const uint8_t RTO_ALPHA_DIV_FACTOR = 3;
const uint8_t RTO_BETA_DIV_FACTOR = 2;
const uint8_t RTO_K_MUL_FACTOR = 2;

class DatagramInformation;

enum datagraminformationstatus_t {
    LIST_STATUS_FREE = 0,
    LIST_STATUS_INITIALIZED = 2
};

class DatagramInformation {
public:
    DatagramInformation();
    ~DatagramInformation();
    inline IPv6Request* getipRequest() { return ipRequestInformation; }
    inline uint16_t getTag() { return datagramTag; }
    inline Ieee802154MacAddress& getsrcMac() {
        return (ipRequestInformation->data.srcMacAddress);
    }
    Ieee802154MacAddress& getdstMac() {
        return (ipRequestInformation->data.dstMacAddress);
    }
    inline IPv6Datagram* getDatagram() {
        return (ipRequestInformation->data.datagram);
    }
    inline BufferInformation* getPayLoadBuffer() { return (payLoadbuffer); }
    void setData(IPv6Request* datagramInformation,
                 BufferInformation* payLoadbuffer, uint16_t datagramTag,
                 LowpanAdaptionLayer* lowpan);
    inline bool isFree() { return (_status == LIST_STATUS_FREE); }
    void tick();
    inline uint16_t getCompressedHeaderSize() { return _compressedHeaderSize; }
    inline void setCompressedHeaderSize(uint16_t compressedHeaderSize) {
        _compressedHeaderSize = compressedHeaderSize;
    }
    inline void setSizeOfCompressedDatagram(uint16_t size) {
        _sizeOfCompressedDatagram = size;
    }
    inline uint16_t getSizeOfCompressedDatagram() {
        return _sizeOfCompressedDatagram;
    }

    inline uint16_t getSizeOfUncompressedDatagram() {
        return ipRequestInformation->data.datagram->getCompleteHeaderLength()
                + ipRequestInformation->data.datagram->getUpperLayerPayloadLength();
    }

    inline void setPtrToFirstUncompressedHeader(FollowingHeader* ptr) {
        _pointerToFirstUncompressedHeader = ptr;
    }
    inline FollowingHeader* getPtrToFirstUncompressedHeader() {
        return _pointerToFirstUncompressedHeader;
    }

    inline bool hasInfo() {
        return ((_compressedHeaderSize != 0xFF) &&
                (_sizeOfCompressedDatagram != 0xFFFF) &&
                (_sizeOfFrirstFragment != 0xFF) &&
                (_sizeOfUncompressedHeader != 0xFFFF));
    }

    void free(bool successfulTransmission);
    inline void increaseNumTransmissions(uint8_t by) {
        _txInfo.increaseNumRetries(by);
    }

    bool areAllFragmentsAcked(uint32_t& ackBitmap);

    inline void setSizeofFirstFragment(uint8_t size) {
        _sizeOfFrirstFragment = size;
    }
    inline uint8_t getSizeofFirstFragment() { return _sizeOfFrirstFragment; }
    inline void setSizeOfUncompressedHeader(uint16_t size) {
        _sizeOfUncompressedHeader = size;
    }
    inline uint16_t getSizeOfUncompressedHeader() {
        return _sizeOfUncompressedHeader;
    }
    inline void setLastFragment(uint8_t seqNum) {
        _lastFragmentForThisDatagram = seqNum;
    }
    inline uint8_t getLastFragment() { return _lastFragmentForThisDatagram; }
    uint32_t getRetransmissionList(uint32_t& receivedAckBitmap);

    void startRTOTimer();
    void stopRTOTimer();
    void decrementRTOTick();

   private:
    void clearFields();
    void resetRTOParameters();
    void updateMeasuredRTTValues();
    uint16_t calculateRTO();
    uint32_t calculateRTTVAR(uint32_t& rttVarOld, uint32_t& srttOld,
                             time_ms_t& measuredRTT);
    uint32_t calculateSRTT(uint32_t& srttOld, time_ms_t& measuredRTT);
    bool isRTOCounterDone();
    uint32_t generateTransmissionListForRTO_RFRAG_AR();

private:
    // there is scope for reducing the info stored some of it can be calculated
    // .. explore later
    IPv6Request* ipRequestInformation;
    BufferInformation* payLoadbuffer;
    uint16_t datagramTag;
    uint8_t _compressedHeaderSize;
    uint16_t _sizeOfCompressedDatagram;
    uint8_t _status;
    FollowingHeader* _pointerToFirstUncompressedHeader;
    uint8_t _sizeOfFrirstFragment;
    uint16_t _sizeOfUncompressedHeader;
    uint8_t _lastFragmentForThisDatagram;
    LlTxInfo _txInfo;
    struct rtt_t {
        uint32_t SRTT;
        uint32_t RTT_VAR;
        time_ms_t timeStamp;
        rtt_t()
            : SRTT(RTO_INITIAL_VALUE),
              RTT_VAR(RTO_INITIAL_VALUE >> 1),
              timeStamp(0) {}
    } _rttParameters;
    uint16_t _rtoTick;
    bool _isRTOenabled;
    bool _isRTOTimeout;            // RTO timeout retransmissions
    LowpanAdaptionLayer* _lowpan;  // bad hack for nw
};

template <uint8_t NumberOfEntries>
class IpRetransmissionList {
   public:
    IpRetransmissionList() {};

    DatagramInformation* addDatagram(IPv6Request* datagramInformation,
                                     BufferInformation* payLoadbuffer,
                                     uint16_t datagramTag,
                                     LowpanAdaptionLayer* lowpan) {
        uint8_t index;
        if (getFreeIndex(index)) {
            _datagram[index].setData(datagramInformation, payLoadbuffer,
                                     datagramTag, lowpan);
            return (_datagram + index);
        } else {
            LOG_DEBUG("No space in Retransmission List");
            // TODO: implement stats collection on failure
            return NULL;
        }
    }

    DatagramInformation* retrieveDatagram(uint16_t& tag,
                                          const Ieee802154MacAddress& dstMac) {
        for (uint8_t i = 0; i < NumberOfEntries; ++i) {
            if (!_datagram[i].isFree() && _datagram[i].getTag() == tag &&
                _datagram[i].getdstMac() == dstMac) {
                return &(_datagram[i]);
            }
        }
        return NULL;
    }

    void tick() {
        for (uint8_t i = 0; i < NumberOfEntries; ++i) {
            if (_datagram[i].isFree() == false) {
                _datagram[i].tick();
            }
            // STATS_COLLECTION
        }
    }
    void decrementRTOTick() {
        for (uint8_t i = 0; i < NumberOfEntries; ++i) {
            if (_datagram[i].isFree() == false) {
                _datagram[i].decrementRTOTick();
            }
        }
    }

    virtual ~IpRetransmissionList() {
        for (uint8_t i = 0; i < NumberOfEntries; ++i) {
            _datagram[i].free(false);
            // STATS_COLLECTION
        }
    };

   private:
    bool getFreeIndex(uint8_t& listIndex) {
        for (uint8_t i = 0; i < NumberOfEntries; ++i) {
            if ((_datagram[i].isFree())) {
                listIndex = i;
                return true;
            }
        }
        return false;
    }

   private:
    DatagramInformation _datagram[NumberOfEntries];
};

} /* namespace cometos_v6 */

#endif /* RETRANSMISSIONLIST_H_ */
