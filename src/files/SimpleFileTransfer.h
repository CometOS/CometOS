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

#ifndef SIMPLEFILETRANSFER_H
#define SIMPLEFILETRANSFER_H

#include "Endpoint.h"
#include "AirString.h"
#include "SegmentedFile.h"
#include "CachedSegmentedFile.h"
#include "Vector.h"


#define PERSISTENT_SEGMENT_SIZE 128
#define PACKET_SEGMENT_SIZE     64

namespace cometos {

class SimpleFileTransfer : public Endpoint {
public:
    SimpleFileTransfer(const char *name = "sft");
    void setFileWrapper(SegmentedFile* file);
    void run(node_t receiver, AirString localFilename, AirString remoteFilename, Callback<void(cometos_error_t,node_t)> finishedCallback);
    void handleIndication(DataIndication* msg);
    Arbiter* getArbiter();
    void setIntervalMS(int intervalMS); // should be set to 0 when using together with a reliable transport layer
                                        // the next packet is only sent when the previous one was acknowledged

private:
    void localFileOpened(cometos_error_t result);
    void handleResponse(DataResponse* resp);
    void dataReadStart();
    void dataReadFinished(cometos_error_t result);
    void close(cometos_error_t result);
    void finalize(cometos_error_t result);
    void msgGranted(DataIndication* msg);
    void msgHandled(DataIndication* msg);
    void fileOpenedFromRemote(cometos_error_t result);

    CallbackTask dataReadTask;

    int intervalMS;

    node_t receiver;
    bool initiator;
    SegmentedFile* persistentFile;
    CachedSegmentedFile cacheFile;
    cometos::AirString localFilename;
    cometos::AirString remoteFilename;
    uint16_t filenameCRC;
    Callback<void(cometos_error_t,node_t)> finishedCallback;
    cometos_error_t finalResult;

    Vector<uint8_t,PACKET_SEGMENT_SIZE> data;
    num_segments_t currentSegment;

    Arbiter arbiter;

    enum class CMD : uint8_t {
        OPEN,
        CLOSE,
        SEGMENT
    };
};

}

#endif


