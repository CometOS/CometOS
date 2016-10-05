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

#include "PacketInformation.h"
#include "lowpan-macros.h"
#include "logging.h"

namespace cometos_v6 {

PacketInformation::~PacketInformation() {

    // we cannot know if the module that sent the request
    // is not already deleted, therefore, we do not send a response but
    // instead do nothing with regard to the request. Whoever sent the
    // request is responsible for its deletion (or non-deletion)

    this->req = NULL;
    setFree(false);
}

void PacketInformation::set(IPv6Request* req,
                 uint16_t tag,
                 uint16_t newTag,
                 uint16_t size,
                 uint8_t fSize)
{
    status.status = 2;
    this->tag = tag;
    this->newTag = newTag;
    this->status.size = size;
    this->req = req;
    txInfo.reset();
    status.transmitted = fSize;
    _lastSeqNumberOfDatagram = INVALID_SEQ_NO;
}

void PacketInformation::setFree(bool success) {
    LOG_DEBUG("Free PI " << this);
    if (req != NULL) {
        IPv6Response::ipv6ResponseCode_t rc;
        if (success) {
            rc = IPv6Response::IPV6_RC_SUCCESS;
        } else {
            rc = IPv6Response::IPV6_RC_FAIL_ALREADY_COUNTED;
        }
        IPv6Response * resp = new IPv6Response(req, rc);
        if (success) {
            txInfo.setSuccess();
        }
        LOG_DEBUG("Response to upper layer: success=" << success);
        resp->set(new LlTxInfo(txInfo));
        cometos_v6::IPv6Request* tmpReq = req;
        req = NULL;
        tmpReq->response(resp);
    } else {
        LOG_WARN("PI setFree without Request");
    }
    status.status = 0;
}

bool PacketInformation::checkTimeout() {
    if (!isFree()) {
        LOG_DEBUG("status=" << (int) status.status << "|dg=" << req->data.datagram);
        status.status--;
        if (isFree() && req->data.datagram != NULL) {
//            delete req->datagram;
//            req->datagram = NULL;
            // signal unsuccessful reassembly
            setFree(false);
            return true;
        }
    }
    return false;
}

fragmentResult_t PacketInformation::addFragment(uint8_t offset, uint8_t length) {
    uint16_t pos = offsetToByteSize(offset);
    if (pos == status.transmitted) {
        status.transmitted += length;
        if (status.transmitted == status.size) {
            // we received the whole datagram; avoid timeout by setting a really high value
            status.status = 0xFF;
        }
        return PI_IN_ORDER;
    } else if (pos < status.transmitted) {
        return PI_ALREADY;
    } else {
        return PI_INVALID_ORDER;
    }
}

void PacketInformation::storeLastSeqNumberOfDatagram(uint8_t sequenceNumber) {
    ASSERT(sequenceNumber < 32);
    if ((_lastSeqNumberOfDatagram == INVALID_SEQ_NO) ||
        (sequenceNumber > _lastSeqNumberOfDatagram)) {
        _lastSeqNumberOfDatagram = sequenceNumber;
    }
}

const Ieee802154MacAddress& PacketInformation::getDstMAC() const {
    ASSERT(req != NULL);
    return req->data.dstMacAddress;
}

const Ieee802154MacAddress& PacketInformation::getSrcMAC() const {
    ASSERT(req != NULL);
    return req->data.srcMacAddress;
}



}  // namespace cometos_v6
