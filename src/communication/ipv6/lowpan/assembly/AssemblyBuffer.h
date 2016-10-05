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

#ifndef ASSEMBLYBUFFER_H_
#define ASSEMBLYBUFFER_H_

#include "logging.h"
#include "lowpanconfig.h"
#include "LowpanBuffer.h"
#include "Ieee802154MacAddress.h"
#include "DatagramReassembly.h"
#include "IPHCDecompressor.h"
#include "FragmentHandler.h"
#include "RoutingBase.h"

namespace cometos_v6 {

class AssemblyBufferBase : public FragmentHandler{
public:
    AssemblyBufferBase(DatagramReassembly* handlers,
                       uint8_t numHandlers,
                       ManagedBuffer* lowpanBuf);

    virtual ~AssemblyBufferBase();

    virtual DatagramReassembly* addFragment(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            uint8_t offset,
            cometos::Airframe& frame,
            bufStatus_t & status);

    virtual DatagramReassembly* addFirstFragment(
             const Ieee802154MacAddress& srcMAC,
             uint16_t tag,
             uint16_t size,
             cometos::Airframe& frame,
             IPHCDecompressor* & dcDatagram,
             bufStatus_t & status);


    virtual DatagramReassembly* addFirstLFFRFragment(
             const Ieee802154MacAddress& srcMAC,
             uint16_t tag,
             uint16_t size,
             cometos::Airframe& frame,
             IPHCDecompressor* & dcDatagram,
             bufStatus_t & status, bool enableImplicitAck);

    virtual DatagramReassembly* addSubsequentLFFRFragment(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            uint8_t offset,
            uint8_t seqNum,
            cometos::Airframe& frame,
            bufStatus_t & status,
            bool enableImplicitAck);


    virtual bool handleRFRAG_ACKMessage(uint16_t& receivedTag,
                                const Ieee802154MacAddress& srcMAC,
                                uint32_t& receivedAckBitmap,
                                bufStatus_t& status, bool enableECN);

    virtual uint8_t tick();

    /**
     * NOTE: this method MUST only be used to artificially reduce the number of
     * reassembly handlers associated with this object. Never set to a value
     * larger than the passed array of reassembly handlers.
     *
     * @param number of entries this object may use for reassembling datagrams
     */
    virtual void setMEntries(uint8_t entries);

protected:
    DatagramReassembly*& getHandlers() {
        return handlers;
    }

private:

    DatagramReassembly* findCorrespondingId(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size);

    DatagramReassembly* getBuffer(
            const Ieee802154MacAddress& srcMAC,
            uint16_t tag,
            uint16_t size,
            bufStatus_t & status);
private:
    DatagramReassembly* handlers;
    uint8_t mEntries;
    ManagedBuffer* lBuffer;
};


class DynAssemblyBuffer : public AssemblyBufferBase {
public:
    DynAssemblyBuffer(ManagedBuffer* buffer, uint8_t MEntries);
    ~DynAssemblyBuffer();

};

/**
 * Actual template class to be instantiated. Actual work is done
 * by AssemblyBufferBase.
 */
template <uint8_t MEntries>
class AssemblyBuffer : public AssemblyBufferBase {
public:
    AssemblyBuffer(ManagedBuffer* buffer) :
        AssemblyBufferBase(handlerArray,
        MEntries,
        buffer)
    {}

    ~AssemblyBuffer() {
        for (uint8_t i = 0; i < MEntries; i++) {
            handlerArray[i].free();
        }
    }

    DatagramReassembly handlerArray[MEntries];
};

//typedef AssemblyBuffer<LOWPAN_SET_ASSEMBLY_ENTRIES> AssemblyBuffer_t;
typedef DynAssemblyBuffer AssemblyBuffer_t;

} /* namespace cometos_v6 */
#endif /* ASSEMBLYBUFFER_H_ */
