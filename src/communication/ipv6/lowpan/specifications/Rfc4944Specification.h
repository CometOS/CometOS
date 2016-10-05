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

#ifndef RFC4944SPECIFICATION_H_
#define RFC4944SPECIFICATION_H_

#include "LowpanVariant.h"
#include "lowpan-macros.h"

namespace cometos_v6 {

class Rfc4944Implementation : public cometos_v6::LowpanVariant {

public:
    Rfc4944Implementation(
        LowpanAdaptionLayerStats& stats,
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

    ~Rfc4944Implementation();

    virtual void parseframe(cometos::Airframe& frame,
                            uint8_t lowpanDispatch,
                            const Ieee802154MacAddress& src,
                            const Ieee802154MacAddress& dst,
                            IPv6DatagramInformation* contextInfo);

    virtual QueueObject* processBufferSpaceAndReturnQueueObject(IPv6Request* req,
                                                        uint16_t tag,
                                                        BufferInformation* buf,
                                                        uint16_t posInBuffer);

    virtual void parseLowpanAndIpHeader(cometos::Airframe& frame,
                                    uint8_t lowpanDispatch,
                                    const Ieee802154MacAddress& src,
                                    const Ieee802154MacAddress& dst,
                                    LowpanFragMetadata& fragMeta,
                                    IPv6Datagram& dg);

private:
    virtual void parseFragmentationHeader(cometos::Airframe& frame,
                                          uint8_t fragDispatch,
                                          LowpanFragMetadata& fragMeta);
    
	void handleFragmentData(cometos::Airframe& frame,
							const LowpanFragMetadata& fragMeta,
                            const Ieee802154MacAddress& src,
                            const Ieee802154MacAddress& dst,
                            IPv6DatagramInformation* contextInfo);

    bool isValidFragmentFrame(const cometos::Airframe& frame, uint8_t head);

    DatagramReassembly* continueReassemblyOfDgram(const Ieee802154MacAddress& srcMAC,
                                                  const LowpanFragMetadata& fragMeta,
                                                  cometos::Airframe& frame);

    void readCommonFragmentationHeader(cometos::Airframe& frame, uint8_t head,
                                        uint16_nbo& dGramSize,
                                        uint16_nbo& dGramTag);

    DatagramReassembly* initiateReassemblyOfDgram(const Ieee802154MacAddress& src,
                                                  uint16_t dGramTag,
                                                  uint16_t dGramSize,
                                                  cometos::Airframe& frame,
                                                  IPHCDecompressor* ipDgramWithoutHdrCompression);
};

} /* namespace cometos_v6 */

#endif /* RFC4944SPECIFICATION_H_ */
