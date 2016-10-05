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

#ifndef DELUGEINFOFILE_H
#define DELUGEINFOFILE_H

#include "cometosError.h"
#include "DelugeInfo.h"
#include "Callback.h"
#include "LoadableTask.h"
#include "Endpoint.h"
#include "AirString.h"
#include "Arbiter.h"
#include "SegFileFactory.h"
#include "DelugeConfig.h"

namespace cometos {

class DelugeInfoFile {
public:
    // Constuctor
    DelugeInfoFile();
    // Destructor
    ~DelugeInfoFile();
    // Starts reading the info file
    void getInfo(Callback<void(cometos_error_t,DelugeInfo*)> callback);
    // Starts writing the info file
    void writeInfo(DelugeInfo* pInfo, Callback<void(cometos_error_t,DelugeInfo*)> callback);

private:
    // writes a segment to the info file
    void writeSegment(cometos_error_t result);
    // reads a segment from the info file into buffer
    void readSegment(cometos_error_t result);
    // Finalizes the read/write operation
    void finalize(cometos_error_t result);
    // parse buffer
    void parseBuffer();
    // last segment written
    void lastSegmentWritten(cometos_error_t result);

private:
    // The callback function
    Callback<void(cometos_error_t,DelugeInfo*)> mDoneCallback;

    // The info opject pointer
    DelugeInfo* pInfo = nullptr;

    // The buffer object
    uint8_t mBuffer[DELUGE_MAX_INFOFILE_SIZE];

    // The current segment that is written/read
    uint16_t mSegmentIndex = 0;

    // The index for the buffer
    uint16_t mBufferByteIndex = 0;

    // The info file name
    AirString mFilename;

    // The file we are operating on
    SegmentedFile* pFile = nullptr;
};

}

#endif
