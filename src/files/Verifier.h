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
 * @author Florian Meier
 */

#ifndef VERIFIER_H
#define VERIFIER_H

#include "cometos.h"
#include "cometosError.h"
#include "AirString.h"
#include "Endpoint.h"
#include "AsyncAction.h"
#include "SegmentedFile.h"
#include "FileProperties.h"

#define VERIFIER_MODULE_NAME "fv"

namespace cometos {

class Verifier : public Endpoint
{
public:
    Verifier(const char* service_name = VERIFIER_MODULE_NAME);

    void handleIndication(DataIndication* msg);
    void setFileWrapper(SegmentedFile* file);
    void initialize();

    Arbiter* getArbiter();

    /**
     * @return COMETOS_ERROR_BUSY until callback is called, COMETOS_PENDING otherwise
     */
    void getLocalFileProperties(AirString filename, Callback<void(FileProperties)> callback);

    /**
     * If another operation is still pending, the old callback will be overwritten!
     * Should not be called with a different callback when old packets might still arrive.
     * Check with node_t and filenameCRC if the call of the callback is the expected one!
     * (This allows for sane operation if packets get lost)
     */
    void getRemoteFileProperties(node_t remote, AirString filename, Callback<void(node_t, FileProperties)> callback);

    FileProperties getLastFileProperties();

    /**
     * CRC calculation
     * (xmodem, polynom is 0x1021)
     */
    static uint16_t updateCRC(uint16_t crc, const uint8_t* data, uint16_t length);

private:
    static const segment_size_t FILE_SEGMENT_SIZE = 128;

    void msgRequestGranted(DataIndication* msg);
    void fileAccessGranted();
    void read(cometos_error_t result);
    void final(cometos_error_t result);
    void sendResult(FileProperties properties);
    FileProperties prop;
    SegmentedFile* file;
    Callback<void(FileProperties)> localCallback;
    Callback<void(node_t, FileProperties)> remoteCallback;
    uint8_t buf[FILE_SEGMENT_SIZE];
    num_segments_t readSegment;
    node_t replyAddr;
    Arbiter arbiter;
    AirString filename;
    ArbiterAction fileRequest;

    enum class CMD : uint8_t {
        REQUEST,
        RESPONSE
    };
};

}

#endif
