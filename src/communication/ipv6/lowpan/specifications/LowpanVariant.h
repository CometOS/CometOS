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

#ifndef LOWPANVARIANT_H_
#define LOWPANVARIANT_H_

#include "IPv6Address.h"
#include "bufferconfig.h"
#include "LowpanAdaptionLayerStructures.h"
#include "QueueObject.h"
#include "QueuePacket.h"
namespace cometos_v6 {

class LowpanVariant {
public:
    LowpanVariant(LowpanAdaptionLayerStats& stats,
                  FragmentHandler& fragmentHandler,
                  ManagedBuffer& buffer,
#ifdef OMNETPP
                  omnetpp::cOutVector& BufferSizeVector, omnetpp::cOutVector& BufferNumElemVector,
#endif
                  IPv6Context (&srcContexts)[16],
                  IPv6Context (&dstContexts)[16],
                  retransmissionList& ipRetransmissionList,
                  LowpanAdaptionLayer* lowpan);
    virtual ~LowpanVariant();

    virtual void parseframe(cometos::Airframe& frame,
                            uint8_t lowpanDispatch,
                            const Ieee802154MacAddress& src,
                            const Ieee802154MacAddress& dst,
                            IPv6DatagramInformation* dgInfo) = 0;

    virtual void parseLowpanAndIpHeader(cometos::Airframe& frame,
                                uint8_t lowpanDispatch,
                                const Ieee802154MacAddress& src,
                                const Ieee802154MacAddress& dst,
                                LowpanFragMetadata& fragMeta,
                                IPv6Datagram& dg) = 0;

    virtual QueueObject* processBufferSpaceAndReturnQueueObject(
        IPv6Request* req, uint16_t tag, BufferInformation* buf,
        uint16_t posInBuffer) = 0;

    virtual void parseFragmentationHeader(cometos::Airframe& frame,
                                          uint8_t fragDispatch,
                                          LowpanFragMetadata& fragMeta) = 0;

    void getSrcDstAddrFromMesh(cometos::Airframe& frame, const uint8_t head,
                               Ieee802154MacAddress& src,
                               Ieee802154MacAddress& dst);

    void retrieveIpBaseHeader(cometos::Airframe& frame,
                          uint8_t lowpanDispatch,
                          const Ieee802154MacAddress& src,
                          const Ieee802154MacAddress& dst,
                          IPv6Datagram& dg);
protected:
    LowpanAdaptionLayerStats& stats;

    FragmentHandler& fragmentHandler;

    ManagedBuffer& buffer;
#ifdef OMNETPP
    omnetpp::cOutVector& BufferSizeVector;
    omnetpp::cOutVector& BufferNumElemVector;
#endif

    IPv6Context (&srcContexts)[16];

    IPv6Context (&dstContexts)[16];

    retransmissionList& _ipRetransmissionList;

    LowpanAdaptionLayer* _lowpan;

    void extractUnfragmentedIP(cometos::Airframe& frame, uint8_t head,
                               const Ieee802154MacAddress& src,
                               const Ieee802154MacAddress& dst,
                               IPv6DatagramInformation* contextInfo);

    void removeHopsLftFieldFrmFrame(cometos::Airframe& frame, uint8_t head);

    void removeSrcAddrFieldFrmFrame(cometos::Airframe& frame, uint8_t head,
                                    Ieee802154MacAddress& src);

    void removeDstAddrFieldFrmFrame(cometos::Airframe& frame, uint8_t head,
                                    Ieee802154MacAddress& dst);

    IPHCDecompressor* getDecompressorFromFrame(
            cometos::Airframe& frame,
            const Ieee802154MacAddress& src,
            const Ieee802154MacAddress& dst,
            const IPv6Context* srcContexts,
            const IPv6Context* dstContexts);

    void updateStatsOnReturnValue(bufStatus_t retVal);

    bool isReassemblyComplete(DatagramReassembly* dGramInfo);
};

} /* namespace cometos_v6 */

#endif /* LOWPANVARIANT_H_ */
