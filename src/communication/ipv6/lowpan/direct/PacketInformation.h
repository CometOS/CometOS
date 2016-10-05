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

#ifndef PACKETINFORMATION_H_
#define PACKETINFORMATION_H_

#include "cometos.h"
#include "MacAbstractionBase.h"
#include "RoutingBase.h"
#include "Ieee802154MacAddress.h"
#include "IPv6Request.h"

namespace cometos_v6 {
const uint8_t INVALID_SEQ_NO = 0xFF;

enum fragmentResult_t : uint8_t {
    PI_IN_ORDER,          ///< new fragment fitted seamlessly
    PI_ALREADY,           ///< new fragment already received
    PI_INVALID_ORDER   ///< new fragment is out of order
};

/**
 * Stores information about a datagram which is being forwarded fragmentwise
 * (without reassembly).
 */
class PacketInformation {
public:
    PacketInformation()
        : newTag(0), tag(0), req(NULL), _lastSeqNumberOfDatagram(INVALID_SEQ_NO) {}
    ~PacketInformation();

    /** Initialize data of datagram in transition
     * @param req request associated with the datagram
     * @param tag value of the incoming tag
     * @param newTag value of the outgoing tag
     * @param size size of the whole datagram as in the 6lowpan header
     * @param fSize number of bytes already transmitted, i.e., the size of
     *              the first fragment
     */
    void set(IPv6Request* req,
             uint16_t tag,
             uint16_t newTag,
             uint16_t size,
             uint8_t fSize);

    void setFree(bool success);
    bool checkTimeout();

    fragmentResult_t addFragment(uint8_t offset, uint8_t length);

    inline IPv6Datagram* decapsulateIPv6Datagram() {
        if (req != NULL) {
            IPv6Datagram* ret = req->data.datagram;
            req->data.datagram = NULL;
            return ret;
        } else {
            return NULL;
        }
    }

    inline IPv6Datagram* getDatagram() const {
        if (req != NULL) {
            return req->data.datagram;
        } else {
            return NULL;
        }
    }

    const IPv6Request* getRequest() const {
        return req;
    }

    const Ieee802154MacAddress& getDstMAC() const;
    const Ieee802154MacAddress& getSrcMAC() const;


    inline uint16_t getTransmitted() const {
        return status.transmitted;
    }

    inline uint16_t getSize() const {
        return status.size;
    }

    inline uint16_t getTag() const {
        return tag;
    }

    inline uint16_t getNewTag() const {
        return newTag;
    }

    inline bool isFree() const {
        return (status.status == 0);
    }

    inline void resetStatus(){
        status.status = 2;
    }

    inline void updateTxInfo(bool success, const cometos::MacTxInfo & info) {
        txInfo.increaseNumRetries(info.numRetries);
        if (success) {
            txInfo.increaseNumTransmissions(1);
        } else {
            txInfo.setFailed();
        }
    }

    const LlTxInfo & getSendInfo() {
        return txInfo;
    }

    void storeLastSeqNumberOfDatagram(uint8_t sequenceNumber);
    inline uint8_t getLastSeqNumberOfDatagram(){
        return _lastSeqNumberOfDatagram;
    }

private:
    uint16_t        newTag;
    uint16_t        tag;
    struct status_t {
        uint16_t size:          12;
        uint16_t transmitted:   12;
        uint8_t  status:        8;
        status_t(): size(0), transmitted(0), status(0) {}
    } status;
    LlTxInfo txInfo;
    IPv6Request * req;
    uint8_t _lastSeqNumberOfDatagram;
};

}

#endif /* PACKETINFORMATION_H_ */
