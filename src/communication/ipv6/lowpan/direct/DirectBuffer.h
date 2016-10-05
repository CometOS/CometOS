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

#ifndef DIRECTBUFFER_H_
#define DIRECTBUFFER_H_

#include "cometos.h"
#include "FragmentHandler.h"
#include "AssemblyBuffer.h"
#include "lowpanconfig.h"
#include "QueueFrame.h"
#include "LFFRFrame.h"
#include "IpForward.h"
#include "LFFRACK.h"

#include "ICMPv6.h"
#include "PacketInformation.h"

namespace cometos_v6 {
const uint32_t NULL_ACK_BITMAP = 0x00000000;
class LowpanAdaptionLayer;

class DirectBuffer : public FragmentHandler {
public:
    DirectBuffer(
            PacketInformation* datagramHandlers,
            uint8_t size,
            LowpanAdaptionLayer* lowpan,
            AssemblyBufferBase* assemblyBuf,
            ManagedBuffer* buffer,
            uint16_t* tag);

    ~DirectBuffer() {}

    void initialize(IpForward * ip);

    /**
     * @param[in] srcMAC
     * @param[in] tag
     * @param[in] size
     * @param[in] offset
     * @param[inout] frame
     * @param[out] status
     */
    DatagramReassembly* addFragment(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            uint8_t offset,
            cometos::Airframe& frame,
            bufStatus_t & status);

    /**
     *
     */
    DatagramReassembly* addFirstFragment(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            cometos::Airframe& frame,
            IPHCDecompressor* & dcDatagram,
            bufStatus_t & status);

    uint8_t tick();

    DatagramReassembly* addFirstLFFRFragment(
            const Ieee802154MacAddress& srcMAC,
             uint16_t tag,
             uint16_t size,
             cometos::Airframe& frame,
             IPHCDecompressor* & dcDatagram,
             bufStatus_t & status, bool enableImplicitAck);
    DatagramReassembly* addSubsequentLFFRFragment( const Ieee802154MacAddress& srcMAC,
                                                    uint16_t tag,
                                                    uint16_t size,
                                                    uint8_t offset,
                                                    uint8_t sequenceNumber,
                                                    cometos::Airframe& frame,
                                                    bufStatus_t & status,
                                                    bool enableImplicitAck);

    bool handleRFRAG_ACKMessage(uint16_t& receivedTag,
                                const Ieee802154MacAddress& srcMAC,
                                uint32_t& receivedAckBitmap,
                                bufStatus_t& status, bool enableECN);

#ifdef LOWPAN_ENABLE_BIGBUFFER
    void setMEntries(uint8_t entries) {
        assembly->setMEntries(entries);
    }
#endif

private:
    PacketInformation* findCorrespondingPI(const Ieee802154MacAddress& srcMAC,
                                           uint16_t tag, uint16_t size);
    PacketInformation* reverseLookUpPI(const Ieee802154MacAddress& receivedMAC,
                                       uint16_t tag);

    void sendLFFRAck(DatagramReassembly* fi, bufStatus_t& status);
    void sendLFFRNullBitmap(const Ieee802154MacAddress& srcMAC, uint16_t& tag,
                            bufStatus_t& status);
    PacketInformation* getFreeHandler();
    bool areAllLFFRFragmentsAcked(PacketInformation* directBufInfoOnDatagram,
                                  const uint32_t& ackBitmap);
    uint32_t generateFullyAckdBitmap(uint8_t sequenceNumber);

protected:
    PacketInformation*& getDatagramHandlers() {
        return datagramHandlers;
    }

private:

    const uint8_t               numEntries;
    AssemblyBufferBase*         assembly;
    LowpanAdaptionLayer *       lowpan;
    ManagedBuffer*              buffer;

    uint16_t*                   nexttag;

    IpForward*                  ip;
    PacketInformation*          datagramHandlers;
};

class DynDirectBuffer : public DirectBuffer {
public:
    DynDirectBuffer(LowpanAdaptionLayer* lowpan,
            AssemblyBufferBase* assemblyBuf,
            ManagedBuffer* buffer,
            uint8_t numDatagramHandlers,
            uint16_t* tag);
   ~DynDirectBuffer();


};

template<uint8_t DEntries>
class DirectBufferImpl : public DirectBuffer {
public:
    DirectBufferImpl(LowpanAdaptionLayer* lowpan,
                     AssemblyBufferBase* assemblyBuf,
                     ManagedBuffer* buffer,
                     uint16_t* tag) :
             DirectBuffer(pktArray,
                          DEntries,
                          lowpan,
                          assemblyBuf,
                          buffer,
                          tag)
    {}

    ~DirectBufferImpl() {}

private:
    PacketInformation pktArray[DEntries];
};


} // namespace cometos_v6

#endif /* DIRECTBUFFER_H_ */
