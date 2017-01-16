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
 * @author Andreas Weigel
 */

#ifndef FRAGHANDLER_H_
#define FRAGHANDLER_H_

#include "DatagramReassembly.h"

namespace cometos_v6 {

#if defined SWIG || defined BOARD_python
enum bufStatus_t {
#else
enum bufStatus_t : uint8_t {
#endif
    BS_SUCCESS,
    BS_SUCCESS_PACKET,
    BS_OUT_OF_SPACE,
    BS_OUT_OF_HANDLERS,
    BS_OUT_OF_SPACE_TINY,
    BS_OUT_OF_SPACE_ASSEMBLY,
    BS_NO_ROUTE,
    BS_DATAGRAM_INVALID,
    BS_FRAGMENT_INVALID_ORDER,
    BS_FRAMGENT_NO_ENTRY,
    BS_QUEUE_FULL,
    BS_NO_HOPS_LEFT,
    BS_OUT_OF_MESSAGES,
    BS_DUPLICATE
}; ///< buffer status to denote errors while processing fragments

/**
 * ABC for all fragment handlers.
 */
class FragmentHandler {
public:
    virtual ~FragmentHandler() {}

    virtual DatagramReassembly* addFragment(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            uint8_t offset,
            cometos::Airframe& frame,
            bufStatus_t & status) = 0;

    virtual DatagramReassembly* addFirstFragment(
             const Ieee802154MacAddress& srcMAC,
             uint16_t tag,
             uint16_t size,
             cometos::Airframe& frame,
             IPHCDecompressor* & dcDatagram,
             bufStatus_t & status) = 0;


    virtual DatagramReassembly* addFirstLFFRFragment(
             const Ieee802154MacAddress& srcMAC,
             uint16_t tag,
             uint16_t size,
             cometos::Airframe& frame,
             IPHCDecompressor* & dcDatagram,
             bufStatus_t & status,
             bool enableImplicitAck) = 0;

    virtual DatagramReassembly* addSubsequentLFFRFragment(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            uint8_t offset,
            uint8_t seqNum,
            cometos::Airframe& frame,
            bufStatus_t & status,
            bool enableImplicitAck) = 0;


    virtual bool handleRFRAG_ACKMessage(uint16_t& receivedTag,
                                const Ieee802154MacAddress& srcMAC,
                                uint32_t& receivedAckBitmap,
                                bufStatus_t& status,
                                bool enableECN) = 0;

    virtual uint8_t tick() = 0;

    /**
     * NOTE: this method MUST only be used to artificially reduce the number of
     * reassembly handlers associated with this object. Never set to a value
     * larger than the passed array of reassembly handlers.
     *
     * @param number of entries this object may use for reassembling datagrams
     */
//    virtual void setMEntries(uint8_t entries) = 0;
};


} // namespace


#endif /* INCOMINGFRAGHANDLER_H_ */
