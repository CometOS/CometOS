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

#ifndef DELUGEHANDLER_H
#define DELUGEHANDLER_H

#include "Verifier.h"
#include "Deluge.h"
#include "DelugeInfoFile.h"
#include "Callback.h"
#include "cometosError.h"
#include "AirString.h"
#include "SegFileFactory.h"

namespace cometos {

class DelugeHandler : public Task {
public:
    // Constructor
    DelugeHandler();
    // Destructor
    ~DelugeHandler();
    // Start the new file command
    cometos_error_t setFile(AirString& filename, Callback<void(cometos_error_t)> finishedCallback);
    // Invoken by timer
    virtual void invoke() override;

    void setDeluge(Deluge* deluge);

    void stop();

private:
    // Origin file opened
    void fileOriginOpened(cometos_error_t result);
    // Destination file opened
    void fileDestinationOpened(cometos_error_t result);
    // start read a segment
    void readSegment();
    // A segment is read
    void segmentRead(cometos_error_t result);
    // A segment is written
    void segmentWritten(cometos_error_t result);
    // Origin file is closed
    void fileOriginClosed(cometos_error_t result);
    // finalize the class
    void finalize(cometos_error_t result);
    // Final comarison between old and new info file
    void infoFileOpened(cometos_error_t result, DelugeInfo* info);


private:
    Deluge* deluge;

    // The origin file
    SegmentedFile* pOriginFile = nullptr;
    // The destination file
    SegmentedFile* pDestinationFile = nullptr;
    // The index for the current segment
    uint16_t mCurrentSegment;
    // The buffer for file operations
    Vector<uint8_t,DELUGE_PACKET_SEGMENT_SIZE>      mBuffer;
    // The crc code for page check
    uint16_t                                        mCRC = 0;
    // The number of pages
    uint8_t                                         mNumberOfPages = 0;
    // The page crc vector
    uint16_t*                                       pPageCRC = nullptr;
    // The info file reader/writer
    DelugeInfoFile                                  mInfoFile;
    // The filename of the infofile
    AirString                                       mFilename;
    // The filename of  the datafile
    AirString                                       mDatafilename;

};

}

#endif
