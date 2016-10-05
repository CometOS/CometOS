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

#ifndef DELUGERX_H
#define DELUGERX_H

#include "SegFileFactory.h"
#include "DelugeConfig.h"
#include "Endpoint.h"

namespace cometos {

class Deluge;

class DelugeRX {
public:
    // Constructor
    DelugeRX(Deluge *pDeluge, uint8_t page);
    // Destructor
    ~DelugeRX();
    // timer
    void timer();
    // Activate RX State
    void activate();
    // Add suitable host
    void addSuitableHost(node_t host);
    // Handle given packet
    void handlePacket(Airframe& frame);
    // Return page
    uint8_t getPage() const;
    // Destroy RX
    void destroy();

private:
    // Called when file is opened
    void onFileOpen(cometos_error_t result);
    // Called when message is sent
    void onMessageSent(cometos_error_t result);
    // Called when packet is written to file
    void onPacketWritten(cometos_error_t result);
    // Called when checking page
    void onPageCheck(cometos_error_t result);
    // Called when the last segement is written
    void onLastSegmentWritten(cometos_error_t result);

    // Return current host
    node_t getHost() const;
    // Send page request
    void sendPageRequest();
    // finalize
    void finalize(cometos_error_t result);

private:
    // The file we are operating on
    SegmentedFile* pFile = nullptr;

    // The filename of the deluge data file stored as AirString
    AirString mFilename;

    // Holds suitable hosts, that can provide packets for this page
    node_t mSuitableHosts[DELUGE_RX_SUITABLE_HOSTS_SIZE];

    // Points to the last suitable host
    uint8_t mSuitableHostIndex = 255;

    // Points to the currently used host
    uint8_t mCurrentHostIndex = 255;

    Deluge* pDeluge = nullptr;

    // The active state
    bool mActive = false;

    // The requested page
    uint8_t mPage;

    // The packets required to receive in order to complete requested page
    uint32_t mPacketsMissing = 0;

    // The CRC Code for the received page
    uint16_t mPageCRC = 0;

    // The CRC Code for page check
    uint16_t mPageCheckCRC = 0;

    // The packet currently checked for page check
    uint8_t mPageCheckPacket = 0;

    bool mPageCheckActive = false;

    // The segment buffer used for file operations
    Vector<uint8_t,DELUGE_PACKET_SEGMENT_SIZE> mBuffer;

    // Received packets since last timer
    uint8_t mPacketsSinceLastTimer = 0;
};

}

#endif
