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

#ifndef QUEUEFRAME_H_
#define QUEUEFRAME_H_

#include "cometos.h"
#include "palId.h"
#include "PacketInformation.h"
#include "LowpanBuffer.h"
#include "QueueFragment.h"
#include "IPHCCompressor.h"

namespace cometos_v6 {

class QueueFrame: public QueueFragment {
public:
    /**
     * Create a QueueFrame.
     *
     * Contains data about a 6LoWPAN fragment and can create a corresponding
     * Airframe on demand.
     *
     * @param directPacket
     *      meta data about the datagram transmission
     * @param buffer
     *      buffer object storing the data for this fragment
     * @param uncompressedPosition
     *      index into the given buffer, where the first uncompressed and
     *      uninterpreted data starts, i.e., data that is NOT contained
     *      in the headers of the IPv6Datagram
     * @param offset
     *      6LoWPAN fragment offset in units of 8 octets
     * @param last
     *      flag indicating if this is the last fragment of a datagram
     * @param typeOfUncontainedNextHeader
     *      type of the next uninterpreted header in the header chain
     *      we do not need to interpret the data within the header, but
     *      have to write the header type in the header preceding it
     *
     */
    QueueFrame(PacketInformation* directPacket,
            BufferInformation* buffer,
            uint8_t uncompressedPosition,
            uint8_t offset,
            bool last = false,
            uint8_t typeOfUncontainedNextHeader = FollowingHeader::NoNextHeaderNumber);
    ~QueueFrame();

    virtual QueueObject::response_t response(bool success, const cometos::MacTxInfo & info);

    virtual void createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragHead,
                             const IPv6Datagram* & dg);

    virtual const Ieee802154MacAddress& getDstMAC() const;
    virtual uint8_t getPRCValue() const;
    virtual uint8_t getSRCValue() const;
    virtual uint8_t getHRCValue() const;
    virtual void logEnqueue() const { LOG_DEBUG("Frame in Queue"); }
    virtual bool canBeDeleted() const;

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const;

    virtual LocalDgId getDgId(bool& valid) const;

    virtual const IPv6Datagram* getDg() const;

    virtual uint8_t currOffset() const;

    virtual uint16_t getCurrDgSize() const;

    virtual inline PacketInformation* getDirectPacket() {
        return directPacket;
    }
    inline bool isSent() const {
        return state.inTransmission;
    }
private:
    PacketInformation*  directPacket;
    uint8_t             offset;
    BufferInformation*  buffer;

    struct state_t {
        bool    inTransmission: 1;
        bool    last: 1;
        state_t(bool s, bool l): inTransmission(s), last(l) {}
    } state;
    uint8_t             typeOfUncontainedNextHeader;
    uint16_t            uncompressedPos;
};

} /* namespace cometos_v6 */
#endif /* QUEUEFRAME_H_ */
