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

#ifndef DELUGETX_H
#define DELUGETX_H

#include "SegFileFactory.h"
#include "DelugeConfig.h"

namespace cometos {

class Deluge;

class DelugeTX {
public:
    // Constructor
    DelugeTX(Deluge *pDeluge, uint8_t page);
    // Destructor
    ~DelugeTX();
    // Add packets
    void addRequestedPackets(uint32_t packets);
    // Prepare packet to send
    void preparePacket(cometos_error_t result);
    // destroy this instance
    void destroy();
    uint8_t getPage() const;

private:
    // Called when message is sent
    void onMessageSent(cometos_error_t result);
    // sends a packet
    void sendPacket(cometos_error_t result);
    // finalize
    void finalize(cometos_error_t result);

private:
    // The main deluge class
    Deluge* pDeluge = nullptr;

    // The requested page
    uint8_t mPage;

    // The file we are operating on
    SegmentedFile *pFile = nullptr;

    // The filename of the deluge data file stored as AirString
    AirString mFilename;

    // The current packet that should be sent
    uint8_t mPacketToSend = 255;

    // The bitvector that holds the packets that should be transmitted
    uint32_t mRequestedPackets = 0;

    // The segment buffer used for file operations
    Vector<uint8_t,DELUGE_PACKET_SEGMENT_SIZE> mBuffer;
};

}

#endif



