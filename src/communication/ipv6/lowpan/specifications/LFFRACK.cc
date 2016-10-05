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

#include <LFFRACK.h>

#define LFFR_ACK_HDR 0xEA
#define LFFR_ACK_HDR_WITH_ECN_INDICATION 0xEB

namespace cometos_v6 {

LFFRAck::LFFRAck(uint32_t acknowledgementBitmap, uint16_t tag,
                 Ieee802154MacAddress dstMAC, bool enableECN)
    : _acknowledgementBitmap(acknowledgementBitmap),
      _tag(tag),
      _destinationMac(dstMAC),
      _enableECN(enableECN) {}

LFFRAck::~LFFRAck() {
    // TODO Auto-generated destructor stub
}

const Ieee802154MacAddress& LFFRAck::getDstMAC() const {
    return _destinationMac;
}

uint8_t LFFRAck::getPRCValue() const {
    ASSERT(false);
    return 0;
}

uint8_t LFFRAck::getSRCValue() const {
    ASSERT(false);
    return 0;
}

uint8_t LFFRAck::getHRCValue() const {
    ASSERT(false);
    return 0;
}

QueueObject::response_t LFFRAck::response(bool success,
                                          const cometos::MacTxInfo& info) {
    return (QueueObject::QUEUE_DELETE_OBJECT);
}

void LFFRAck::createFrame(cometos::Airframe& frame,
                          uint8_t maxSize,
                          LowpanFragMetadata& fragMeta,
                          const IPv6Datagram* & dg) {
    uint8_t ackFlag = (uint8_t)LFFR_ACK_HDR;
    if (_enableECN) {
        ackFlag = (uint8_t)LFFR_ACK_HDR_WITH_ECN_INDICATION;
    }
    frame << _acknowledgementBitmap << _tag << ackFlag;

    // we do not consider an LFFR ACK to be fragment of a "real" datagram
    dg = nullptr;
    fragMeta.dgSize = frame.getLength();
    fragMeta.size = frame.getLength();
    fragMeta.offset = 0;
    fragMeta.tag = _tag.getUint16_t();
}

bool LFFRAck::belongsTo(Ieee802154MacAddress const & src,
        Ieee802154MacAddress const & dst,
        uint16_t tag,
        uint16_t size) const {
    return _destinationMac == dst
            && this->_tag.getUint16_t() == tag;
}

LocalDgId LFFRAck::getDgId(bool& valid) const {
    ASSERT(false);
    valid = true;
    return LocalDgId(_destinationMac, _destinationMac, _tag.getUint16_t(), 0);
}

} /* namespace cometos_v6 */
