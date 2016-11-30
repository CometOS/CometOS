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

#include "UncompressOTAP.h"
#include "palRS485.h"
#include "Verifier.h"

using namespace cometos;

static uint8_t fromBuffer[2][OTAP_SEGMENT_SIZE]; // two buffers to handle overlapping of segments
static uint8_t toBuffer[OTAP_SEGMENT_SIZE];

UncompressOTAP::UncompressOTAP()
: fsm_t(&UncompressOTAP::stateIdle), otapfd(-1)
{
    fsm_t::run();
}

Arbiter* UncompressOTAP::getArbiter()
{
    return &arbiter;
}

void UncompressOTAP::run(cometos::SegmentedFile* fromFile, initCallback_t initCallback, writeCallback_t writeCallback, finishCallback_t finishCallback)
{
    getArbiter()->assertRunning();
    this->fromFile = fromFile;
    this->initCallback = initCallback;
    this->writeCallback = writeCallback;
    this->finishCallback = finishCallback;
    this->otapcrc = 0;

    UncompressEvent e(UncompressEvent::RUN_SIGNAL);
    dispatch(e);
}

void UncompressOTAP::readSegment()
{
    segment_size_t segmentSize = fromFile->getSegmentSize(segmentToRead);
    fromFile->read(fromBuffer[segmentToRead%2], segmentSize, segmentToRead, CALLBACK_MET(&UncompressOTAP::segmentRead,*this));
}

void UncompressOTAP::processHeader()
{
    memcpy(&otaphdr,fromBuffer[0],sizeof(otaphdr));

    getCout() << "Magic: 0x" << hex << otaphdr.magic_number << " first_addr: 0x" << otaphdr.first_addr << " last_addr: 0x" << otaphdr.last_addr << dec << " device: " << (uint16_t)otaphdr.device << " header size: " << sizeof(otaphdr) << " payload CRC: 0x" << hex << otaphdr.payload_crc << dec << endl;

    UncompressEvent e(UncompressEvent::HEADER_PROCESSED_SIGNAL);
    e.result = COMETOS_SUCCESS;
    dispatch(e);
}

void UncompressOTAP::externalDone(cometos_error_t result)
{
    UncompressEvent e(UncompressEvent::EXTERNAL_DONE_SIGNAL);
    e.result = result;
    dispatch(e);
}

void UncompressOTAP::writeSegment()
{
    segment_size_t segmentSize = OTAP_SEGMENT_SIZE;
    if(segmentToRead-1 == segmentToWrite+1) {
        // last read segment (segmentToRead-1) is one ahead of the segmentToWrite
        memcpy(toBuffer, fromBuffer[segmentToRead%2]+sizeof(otaphdr), OTAP_SEGMENT_SIZE-sizeof(otaphdr));     // read at the read option before the previous one
        memcpy(toBuffer+OTAP_SEGMENT_SIZE-sizeof(otaphdr), fromBuffer[(segmentToRead+1)%2], sizeof(otaphdr)); // read at the previous read operation
    }
    else if(segmentToRead-1 == segmentToWrite) {
        // last segment
        segmentSize = (otaphdr.last_addr-otaphdr.first_addr+1)%OTAP_SEGMENT_SIZE;
        memcpy(toBuffer, fromBuffer[(segmentToRead+1)%2]+sizeof(otaphdr), segmentSize);                  // read at the previous read operation
    }
    else {
        ASSERT(false);
    }

    writeCallback(toBuffer, segmentSize, segmentToWrite, CALLBACK_MET(&UncompressOTAP::segmentWritten,*this));
}

void UncompressOTAP::segmentRead(cometos_error_t result)
{
    // calculate checksum of otap file
    segment_size_t segmentSize = fromFile->getSegmentSize(segmentToRead);
    otapcrc = Verifier::updateCRC(otapcrc, fromBuffer[segmentToRead%2], segmentSize, true);

    segmentToRead++;

    UncompressEvent e(UncompressEvent::SEGMENT_READ_SIGNAL);
    e.result = result;
    dispatch(e);
}

void UncompressOTAP::segmentWritten(cometos_error_t result)
{
    segmentToWrite++;
    UncompressEvent e(UncompressEvent::SEGMENT_WRITTEN_SIGNAL);
    e.result = result;
    dispatch(e);
}

fsmReturnStatus UncompressOTAP::stateIdle(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    case UncompressEvent::RUN_SIGNAL:
        segmentToRead = 0;
        segmentToWrite = 0;
        return transition(&UncompressOTAP::stateReadFirstSegment);
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}

fsmReturnStatus UncompressOTAP::stateReadFirstSegment(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
        actionTask.setCallback(CALLBACK_MET(&UncompressOTAP::readSegment,*this));
        getScheduler().add(actionTask);
        return FSM_HANDLED;
    case UncompressEvent::SEGMENT_READ_SIGNAL:
        if(event.result == COMETOS_SUCCESS) {
            return transition(&UncompressOTAP::stateProcessHeader);
        }
        else {
            finishResult = event.result;
            return transition(&UncompressOTAP::stateFinish);
        }
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}

fsmReturnStatus UncompressOTAP::stateProcessHeader(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
        actionTask.setCallback(CALLBACK_MET(&UncompressOTAP::processHeader,*this));
        getScheduler().add(actionTask);
        return FSM_HANDLED;
    case UncompressEvent::HEADER_PROCESSED_SIGNAL:
        if(event.result == COMETOS_SUCCESS) {
            return transition(&UncompressOTAP::stateInitWriter);
        }
        else {
            finishResult = event.result;
            return transition(&UncompressOTAP::stateFinish);
        }
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}

fsmReturnStatus UncompressOTAP::stateInitWriter(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
        initCallback(otaphdr, CALLBACK_MET(&UncompressOTAP::externalDone,*this));
        return FSM_HANDLED;
    case UncompressEvent::EXTERNAL_DONE_SIGNAL:
        if(event.result == COMETOS_SUCCESS) {
            return transition(&UncompressOTAP::stateReadSegment);
        }
        else {
            finishResult = event.result;
            return transition(&UncompressOTAP::stateFinish);
        }
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}

fsmReturnStatus UncompressOTAP::stateReadSegment(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
        actionTask.setCallback(CALLBACK_MET(&UncompressOTAP::readSegment,*this));
        getScheduler().add(actionTask);
        return FSM_HANDLED;
    case UncompressEvent::SEGMENT_READ_SIGNAL:
        if(event.result != COMETOS_SUCCESS) {
            getCout() << "Can not uncompress image" << endl;
            finishResult = event.result;
            return transition(&UncompressOTAP::stateFinish);
        }
        else {
            return transition(&UncompressOTAP::stateWriteSegment);
        }
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}

fsmReturnStatus UncompressOTAP::stateWriteSegment(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
        actionTask.setCallback(CALLBACK_MET(&UncompressOTAP::writeSegment,*this));
        getScheduler().add(actionTask);
        return FSM_HANDLED;
    case UncompressEvent::SEGMENT_WRITTEN_SIGNAL:
        if(event.result != COMETOS_SUCCESS) {
            getCout() << "Can not write uncompressed image" << endl;
            finishResult = event.result;
            return transition(&UncompressOTAP::stateFinish);
        }
        else {
            num_segments_t segmentsToWrite = (((otaphdr.last_addr-otaphdr.first_addr+1)-1)/OTAP_SEGMENT_SIZE)+1;

            // segment pending?
            if(segmentToRead < fromFile->getNumSegments()) {
                return transition(&UncompressOTAP::stateReadSegment);
            }
            else if(segmentToWrite < segmentsToWrite) {
                return transition(&UncompressOTAP::stateWriteSegment);
            }
            else {
                // print checksum
                getCout() << hex << "Checksum of otap file: 0x" << otapcrc << endl;
                return transition(&UncompressOTAP::stateFinish);
            }
        }
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}

fsmReturnStatus UncompressOTAP::stateFinish(UncompressEvent& event)
{
    switch(event.signal) {
    case UncompressEvent::ENTRY_SIGNAL:
        if(finishCallback) {
            finishCallback(finishResult, otaphdr, otapcrc, CALLBACK_MET(&UncompressOTAP::externalDone,*this));
        }
        return FSM_HANDLED;
    case UncompressEvent::EXTERNAL_DONE_SIGNAL:
        return transition(&UncompressOTAP::stateIdle);
    case UncompressEvent::EXIT_SIGNAL:
        return FSM_IGNORED;
    default:
        ASSERT(false);
        return FSM_IGNORED;
    }
}
