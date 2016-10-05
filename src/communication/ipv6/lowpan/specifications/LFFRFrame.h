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

#ifndef LFFRFRAME_H_
#define LFFRFRAME_H_

#include "QueueObject.h"
#include "PacketInformation.h"
#include "LowpanBuffer.h"
#include "QueueFragment.h"
#include "IPHCCompressor.h"
#include "LFFRObject.h"

namespace cometos_v6 {

class LFFRFrame : public LFFRObject, public QueueFragment {
    PacketInformation* directPacket;
    BufferInformation* buffer;
    uint8_t _dataGrmOffset;
    uint8_t _sequenceNumber;
    bool _enableImplicitAck;
    uint8_t _typeOfUncontainedNextHeader;
//    uint8_t _uncompressedPos;

   public:
    LFFRFrame(PacketInformation* directPacket,
              BufferInformation* buffer,
              uint8_t uncompressedPos,
              uint8_t dataGrmOffset,
              uint8_t sequenceNumber,
              bool enableImplicitAck,
              uint8_t typeOfUncontainedNextHeader = FollowingHeader::NoNextHeaderNumber);
    virtual ~LFFRFrame();

    QueueObject::response_t response(bool success,
                                     const cometos::MacTxInfo& info);
    virtual void createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragHead,
                             const IPv6Datagram* & dg);

    virtual const Ieee802154MacAddress& getDstMAC() const;
    virtual uint8_t getPRCValue() const;
    virtual uint8_t getSRCValue() const;
    virtual uint8_t getHRCValue() const;
    virtual bool canBeDeleted() const;
    inline virtual void logEnqueue() const { LOG_DEBUG("LFFR Frame in Queue"); }

    inline virtual PacketInformation* getDirectPacket() {
        return directPacket;
    }

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const;

    virtual LocalDgId getDgId(bool& valid) const;

    virtual uint8_t currOffset() const;

    virtual uint16_t getCurrDgSize() const;

    virtual const IPv6Datagram* getDg() const;
};

} /* namespace cometos_v6 */

#endif /* LFFRFRAME_H_ */
