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

#ifndef LFFRSPECIFICATION_H_
#define LFFRSPECIFICATION_H_

#include "LowpanVariant.h"
#include "lowpan-macros.h"
#include "LFFRPacket.h"

namespace cometos_v6 {

class LFFRImplementation : public cometos_v6::LowpanVariant {
    friend class LFFRSpecificationTest;
    const uint8_t RFRAG;
    const uint8_t RFRAG_AR;
    const uint8_t RFRAG_ACK;
    const uint8_t RFRAG_AEC;

public:
    LFFRImplementation(LowpanAdaptionLayerStats& stats,
                       FragmentHandler& fragmentHandler,
                       ManagedBuffer& buffer,
#ifdef OMNETPP
                       omnetpp::cOutVector& BufferSizeVector,
                       omnetpp::cOutVector& BufferNumElemVector,
#endif
                       IPv6Context (&srcContexts)[16],
                       IPv6Context (&dstContexts)[16],
                       retransmissionList& ipRetransmissionList,
                       LowpanAdaptionLayer* lowpan);

    virtual ~LFFRImplementation();

    virtual void parseframe(cometos::Airframe& frame,
                            uint8_t head,
                            const Ieee802154MacAddress& src,
                            const Ieee802154MacAddress& dst,
                            IPv6DatagramInformation* contextInfo);

    virtual void parseLowpanAndIpHeader(cometos::Airframe& frame,
                                uint8_t lowpanDispatch,
                                const Ieee802154MacAddress& src,
                                const Ieee802154MacAddress& dst,
                                LowpanFragMetadata& fragMeta,
                                IPv6Datagram& dg);

    QueueObject* processBufferSpaceAndReturnQueueObject(IPv6Request* req,
                                                        uint16_t tag,
                                                        BufferInformation* buf,
                                                        uint16_t posInBuffer);

    void parseFragmentationHeader(cometos::Airframe& frame,
                                  uint8_t fragDispatch,
                                  LowpanFragMetadata& fragMeta);

private:
	void handleLFFRFragment(cometos::Airframe& frame, uint8_t head,
	                        const Ieee802154MacAddress& src,
	                        const Ieee802154MacAddress& dst,
                            IPv6DatagramInformation* contextInfo);
    inline bool isRFRAG_AR(uint8_t head) { return (head == RFRAG_AR); }
    inline bool isRFRAG_AEC(uint8_t head) { return (head == RFRAG_AEC); }
    inline bool isRFRAG(uint8_t head) {
        return ((head == RFRAG) || (head == RFRAG_AR));
    }
    void handleRFRAG(cometos::Airframe& frame,
                     const Ieee802154MacAddress& src,
                     const Ieee802154MacAddress& dst,
                     IPv6DatagramInformation* contextInfo,
                     bool isRFRAG_AR = false);

    void handleRFRAG_ACKMessage(cometos::Airframe& frame,
                                const Ieee802154MacAddress& src,
                                bool isRFRAG_AEC = false);

    bool isValidFragmentFrame(const cometos::Airframe& frame);


    DatagramReassembly* parseFirstFragment(cometos::Airframe& frame,
                                           const Ieee802154MacAddress& src,
                                           const Ieee802154MacAddress& dst,
                                           uint8_t dGramOffset,
                                           uint16_t dGramTag,
                                           uint16_t dGramSize,
                                           bool enableImplicitAck);

    DatagramReassembly* initiateReassemblyOfDgram(const Ieee802154MacAddress& src,
                                                  uint16_t dGramTag,
                                                  uint16_t dGramSize,
                                                  cometos::Airframe& frame,
                                                  IPHCDecompressor* ipDgramWithoutHdrCompression,
                                                  uint8_t offset,
                                                  bool enableImplicitAck);

    DatagramReassembly* parseSubsequentFragment(cometos::Airframe& frame,
                                                const Ieee802154MacAddress& src,
                                                const Ieee802154MacAddress& dst,
                                                uint8_t dGramOffset,
                                                uint16_t dGramTag,
                                                uint16_t dGramSize,
                                                uint8_t sequenceNumber,
                                                bool isRFRAG_AR);

    bool searchFragmentBufferAndFwdACK(uint16_t& receivedTag,
                                       uint32_t& receivedAckBitmap,
                                       const Ieee802154MacAddress& src,
                                       bufStatus_t& status, bool isRFRAG_AEC);
};

} /* namespace cometos_v6 */

#endif /* LFFRSPECIFICATION_H_ */
