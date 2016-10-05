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


#ifndef QUEUEOBJECT_H_
#define QUEUEOBJECT_H_

#include "cometos.h"
#include "lowpan-macros.h"
#include "Airframe.h"
#include "RoutingBase.h"
#include "MacAbstractionBase.h"
#include "Ieee802154MacAddress.h"
#include "FragMetadata.h"
#include "logging.h"

namespace cometos_v6 {

class LocalDgId {
public:
    LocalDgId(const Ieee802154MacAddress & src,
              const Ieee802154MacAddress & dst,
              uint16_t tag,
              uint16_t size) :
          src(src),
          dst(dst),
          tag(tag),
          size(size)
    {}

    Ieee802154MacAddress src;
    Ieee802154MacAddress dst;
    uint16_t tag;
    uint16_t size;
};

bool operator==(LocalDgId const & lhs, LocalDgId const & rhs);

class QueueObject {
public:
#if defined SWIG || defined BOARD_python
    enum response_t {
#else
    enum response_t : uint8_t {
#endif
        QUEUE_KEEP,
        QUEUE_DELETE_OBJECT,
        QUEUE_DELETE_SIMILAR,
        QUEUE_PACKET_FINISHED
    }; ///< responses of the method response

    QueueObject() {};

    virtual ~QueueObject() {};

    /**
     * this method gets called by the queue, after the mac response arrives.
     */
    virtual response_t response(bool success, const cometos::MacTxInfo & info) = 0;

    /**
     * fills an Airframe with the next fragment of a packet
     * @param frame
     *   Airframe structure to be filled with data
     * @param maxSize
     *   maximum payload data available in this Airframe
     * @param[out] fragMeta
     *
     * @param[out] dg
     *
     */
    virtual void createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragMeta,
                             const IPv6Datagram* & dg) = 0;

    /**
     * returns the destination MAC Address of the packet
     */
    virtual const Ieee802154MacAddress& getDstMAC() const = 0;

    /**
     * returns the rate of how much of the packet has been transmitted between
     * 0 and 255.
     */
    virtual uint8_t getPRCValue() const = 0;

    /**
     * returns the rate of how much has been transmitted compared with the MTU
     */
    virtual uint8_t getSRCValue() const = 0;

    /**
     * returns the rate of how many hops the packet has already taken by the
     * initial hops limit
     */
    virtual uint8_t getHRCValue() const = 0;

    /**
     * returns 0 for QueueObject, 1 for QueuePacket and 2 for QueueFrame
     */
    virtual void logEnqueue() const = 0;

    /**
     * Used to delete coresponding fragments in the queue.
     */
    virtual bool canBeDeleted() const { return false; }

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const = 0;

    virtual LocalDgId getDgId(bool& valid) const = 0;

    virtual const IPv6Datagram* getDg() const = 0;

    virtual uint8_t currOffset() const = 0;

    virtual uint16_t getCurrDgSize() const = 0;

    virtual bool representsDatagram() const = 0;

    virtual bool nextFrameReady() {
        return true;
    }

protected:
    static void addFragmentHeader(cometos::Airframe& frame, uint16_nbo& tag, uint16_t size, uint8_t offset, bool congestionStatus) {
        // TODO byte order
        if (offset > 0) {
            frame << offset;
        }
        frame << tag << uint8_t(size & 0xFF);

        uint8_t dispatch;
        if (offset > 0) {
            dispatch = (uint8_t)(0xE0 | ((size >> 8) & 0x07));
        } else {
            dispatch = (uint8_t)(0xC0 | ((size >> 8) & 0x07));
        }
        LOG_DEBUG("c=" << congestionStatus << "|dispatch=" << cometos::hex << (int) dispatch);
        dispatch |= ((uint8_t)congestionStatus) << LOWPAN_CONGESTION_SHIFT;
        LOG_DEBUG("dispatch=" << cometos::hex << (int) dispatch);
        frame << dispatch;
    }

};

} /* namespace cometos_v6 */
#endif /* QUEUEOBJECT_H_ */
