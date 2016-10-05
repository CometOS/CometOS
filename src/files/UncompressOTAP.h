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

#ifndef UncompressOTAP_H
#define UncompressOTAP_H

#include "OutputStream.h"
#include "AirString.h"
#include "Callback.h"
#include "FSM.h"
#include "otap_header.h"
#include "cometosError.h"
#include "SegmentedFile.h"

namespace cometos {

static const int OTAP_SEGMENT_SIZE = 256;

class UncompressEvent : public FSMEvent {
public:
    enum : uint8_t {
        RUN_SIGNAL = USER_SIGNAL_START,
        GRANTED_SIGNAL,
        EXTERNAL_DONE_SIGNAL,
        SEGMENT_READ_SIGNAL,
        HEADER_PROCESSED_SIGNAL,
        SEGMENT_WRITTEN_SIGNAL,
        FILE_CLOSED_SIGNAL
    };

    UncompressEvent(uint8_t signal)
    : FSMEvent(signal) {
    }

    UncompressEvent()
    : FSMEvent(EMPTY_SIGNAL) {
    }

    cometos_error_t result;
};

class UncompressOTAP : private FSM<UncompressOTAP,UncompressEvent> {
public:
    typedef FSM<UncompressOTAP,UncompressEvent> fsm_t;
    typedef cometos::Callback<void(const otap_header& hdr, cometos::Callback<void(cometos_error_t result)> done)> initCallback_t;
    typedef cometos::Callback<void(uint8_t *data, segment_size_t segmentSize, num_segments_t segment, cometos::Callback<void(cometos_error_t result)> done)> writeCallback_t;
    typedef cometos::Callback<void(cometos_error_t error, otap_header hdr, uint16_t otapCRC, cometos::Callback<void(cometos_error_t result)> done)> finishCallback_t;

    UncompressOTAP();
    void run(cometos::SegmentedFile* from, initCallback_t initCallback, writeCallback_t writeCallback, finishCallback_t finishCallback);

    cometos::Arbiter* getArbiter();

private:
    // states
    fsmReturnStatus stateIdle(UncompressEvent& event);
    fsmReturnStatus stateReadFirstSegment(UncompressEvent& event);
    fsmReturnStatus stateProcessHeader(UncompressEvent& event);
    fsmReturnStatus stateInitWriter(UncompressEvent& event);
    fsmReturnStatus stateReadSegment(UncompressEvent& event);
    fsmReturnStatus stateWriteSegment(UncompressEvent& event);
    fsmReturnStatus stateFinish(UncompressEvent& event);

    // actions
    //void openFromFile();
    //void openToFile();
    void readSegment();
    void writeSegment();
    void processHeader();

    // callbacks
    //void fileOpened(cometos_error_t result);
    void segmentRead(cometos_error_t result);
    void segmentWritten(cometos_error_t result);
    void fileClosed(cometos_error_t result);
    void externalDone(cometos_error_t result);

    // data path
    cometos_error_t finishResult;
    cometos::num_segments_t segmentToRead;
    cometos::num_segments_t segmentToWrite;

    // firmware parameters
    struct otap_header otaphdr;

    cometos::SegmentedFile* fromFile;
    cometos::Arbiter arbiter;

    initCallback_t initCallback;
    writeCallback_t writeCallback;
    finishCallback_t finishCallback;

    cometos::CallbackTask actionTask;

    int otapfd;
    uint16_t otapcrc;
};

}

#endif
