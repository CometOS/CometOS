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

#ifndef LFFRACK_H_
#define LFFRACK_H_

#include <QueueObject.h>

namespace cometos_v6 {

class LFFRAck : public cometos_v6::QueueObject {
public:
    static const uint8_t RFRAG_ACK_SIZE = 7;

    LFFRAck(uint32_t acknowledgementBitmap, uint16_t tag,
            Ieee802154MacAddress dstMAC, bool enableECN);
    virtual ~LFFRAck();
    virtual const Ieee802154MacAddress& getDstMAC() const;
    virtual uint8_t getPRCValue() const;
    virtual uint8_t getSRCValue() const;
    virtual uint8_t getHRCValue() const;
    QueueObject::response_t response(bool success,
                                     const cometos::MacTxInfo& info);
    void createFrame(cometos::Airframe& frame,
                     uint8_t maxSize,
                     LowpanFragMetadata& fHead,
                     const IPv6Datagram* & dg);
    inline virtual void logEnqueue() const { LOG_DEBUG("LFFR Ack Msg in Queue"); }

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const;

    virtual LocalDgId getDgId(bool& valid) const;

    virtual const IPv6Datagram* getDg() const {
        return NULL;
    }

    virtual uint8_t currOffset() const {
        return 0;
    }

    virtual uint16_t getCurrDgSize() const {
        return RFRAG_ACK_SIZE;
    }

    virtual bool representsDatagram() const {
        return true;
    }

private:
    uint32_nbo _acknowledgementBitmap;
    uint16_nbo _tag;
    Ieee802154MacAddress _destinationMac;
    bool _enableECN;
};

} /* namespace cometos_v6 */

#endif /* LFFRACK_H_ */
