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

#ifndef DSMEMESSAGE_H
#define DSMEMESSAGE_H

#include "DSMEMessageElement.h"
#include <stdint.h>
#include "Airframe.h"
#include "IEEE802154eMACHeader.h"

#include "helper/AirframeBuffer.h"

#include "IDSMEMessage.h"

namespace dsme {

class DSMEPlatform;
class DSMEMessageBuffer;

class DSMEMessage : public IDSMEMessage {
public:
    void prependFrom(DSMEMessageElement* msg) override;

    void decapsulateTo(DSMEMessageElement* msg) override;

    void copyTo(DSMEMessageElement* msg) override;

    uint8_t getByte(uint8_t pos) override {
        return frame->getByte(pos);
    }

    void setLength(uint8_t length) override {
        frame->setLength(length);
        return;
    }

    uint8_t getLength() override {
        return frame->getLength();
    }

    uint8_t* getData() override {
        return frame->getData();
    }

    uint32_t getStartOfFrameDelimiterSymbolCounter() override {
        return this->sfdTimestamp;
    }

    uint32_t getReceptionSymbolCounter() override {
        return this->sfdTimestamp + 2 * (this->getHeader().getSerializationLength() + this->frame->getLength()) + 2; // 2 Symbols for PHY header
    }

    uint16_t getTotalSymbols() override {
        return (macHdr.getSerializationLength() + frame->getLength() + 2 // FCS
                + 4 // Preamble
                + 1 // SFD
                + 1 // PHY Header
        ) * (uint16_t) 2; // 4 bit per symbol
    }

    IEEE802154eMACHeader& getHeader() override {
        return macHdr;
    }

private:
    cometos::Airframe* frame;

    IEEE802154eMACHeader macHdr;

    DSMEMessage() :
            frame(nullptr) {
    }

    DSMEMessage(cometos::Airframe* frame) :
            frame(frame) {
    }

    ~DSMEMessage() {
        if (frame != nullptr) {
            AirframeBuffer::getInstance().release(frame);
        }
    }

    uint32_t sfdTimestamp;

    friend class DSMEPlatform;
    friend class DSMEMessageElement;
    friend class DSMEMessageBuffer;

    cometos::Airframe* getSendableCopy() {
        DSMEMessage msg(frame->getDeepCopy());
        macHdr.prependTo(&msg);
        cometos::Airframe* f = msg.frame;
        msg.frame = nullptr;
        return f;
    }

    cometos::Airframe* decapsulateAirframe() {
        cometos::Airframe* f = frame;
        frame = nullptr;
        return f;
    }
};

}

#endif
