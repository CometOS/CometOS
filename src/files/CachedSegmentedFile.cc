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

#include "CachedSegmentedFile.h"
#include "logging.h"

using namespace cometos;

CachedSegmentedFile::CachedSegmentedFile()
: SegmentedFile(),
  fsm_t(&CachedSegmentedFile::stateIdle),
  dispatchCallback(CALLBACK_MET(&CachedSegmentedFile::dispatcher, *this)),
  cache(NULL),
  cacheSize(-1),
  cachedFile(NULL),
  cacheDirty(true)
{
    dispatchTask.setCallback(dispatchCallback);
}

CachedSegmentedFile::~CachedSegmentedFile()
{
    if(cache != NULL) {
        delete[] cache;
        cache = NULL;
    }
}

void CachedSegmentedFile::setCachedFile(SegmentedFile* cachedFile)
{
    ASSERT_RUNNING(getArbiter());

    palExec_atomicBegin();
    getArbiter()->release(); // release previous arbiter
    this->cachedFile = cachedFile;
    ASSERT(getArbiter()->requestImmediately() == COMETOS_SUCCESS); // request new arbiter
    palExec_atomicEnd();
}

Arbiter* CachedSegmentedFile::getArbiter()
{
    if(cachedFile != nullptr) {
        return cachedFile->getArbiter();
    }
    else {
        return SegmentedFile::getArbiter();
    }
}

bool CachedSegmentedFile::isOpen()
{
    ASSERT(cachedFile);
    return cachedFile->isOpen();
}

void CachedSegmentedFile::open(file_identifier_t fileIdentifier, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen)
{
    ASSERT_RUNNING(getArbiter());

    ASSERT(cachedFile);
    cachedSegment = -1;
    cachedFile->open(fileIdentifier, fileSize, finishedCallback, removeBeforeOpen);
}


void CachedSegmentedFile::write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback)
{
    ASSERT_RUNNING(getArbiter());

    if(!this->setStateIfIdle(&CachedSegmentedFile::stateIdle, &CachedSegmentedFile::stateWriteIfNecessary)) {
        ASSERT(false); // not possible when using arbiter correctly
    }

    LOG_INFO("CachedSegmentedFile::write " << cometos::dec << dataLength << " bytes to " << segment);
    for(int i = 0; i < dataLength; i++) {
        LOG_INFO(data[i]);
    }

    ASSERT(segment < getNumSegments());
    ASSERT(dataLength == this->getSegmentSize(segment));

    currentOperation = OPERATION::WRITE;

    externalData = data;
    externalSegment = segment;
    externalCallback = finishedCallback;

    dispatchTask.load(COMETOS_SUCCESS);
    getScheduler().add(dispatchTask);
}

void CachedSegmentedFile::flush(Callback<void(cometos_error_t result)> finishedCallback) {
    ASSERT_RUNNING(getArbiter());

    if(!this->setStateIfIdle(&CachedSegmentedFile::stateIdle, &CachedSegmentedFile::stateWriteIfNecessary)) {
        ASSERT(false); // not possible when using arbiter correctly
    }

    LOG_INFO("CachedSegmentedFile: Flushing the cache");

    currentOperation = OPERATION::WRITE_CACHE;

    externalCallback = finishedCallback;

    dispatchTask.load(COMETOS_SUCCESS);
    getScheduler().add(dispatchTask);
}

void CachedSegmentedFile::read(uint8_t* data, segment_size_t dataLength, num_segments_t segment,
        Callback<void(cometos_error_t result)> finishedCallback)
{
    ASSERT_RUNNING(getArbiter());

    if(!this->setStateIfIdle(&CachedSegmentedFile::stateIdle, &CachedSegmentedFile::stateWriteIfNecessary)) {
        ASSERT(false); // not possible when using arbiter correctly
    }

    ASSERT(segment < getNumSegments());
    ASSERT(dataLength == this->getSegmentSize(segment));

    currentOperation = OPERATION::READ;

    externalData = data;
    externalSegment = segment;
    externalCallback = finishedCallback;

    dispatchTask.load(COMETOS_SUCCESS);
    getScheduler().add(dispatchTask);
}

void CachedSegmentedFile::writeCache(Callback<void(cometos_error_t result)> finishedCallback)
{
    ASSERT_RUNNING(getArbiter());

    if(!this->setStateIfIdle(&CachedSegmentedFile::stateIdle, &CachedSegmentedFile::stateWriteIfNecessary)) {
        ASSERT(false); // not possible when using arbiter correctly
    }

    currentOperation = OPERATION::WRITE_CACHE;

    externalCallback = finishedCallback;

    dispatchTask.load(COMETOS_SUCCESS);
    getScheduler().add(dispatchTask);
}

void CachedSegmentedFile::close(Callback<void(cometos_error_t result)> finishedCallback)
{
    closeCallback = finishedCallback;
    writeCache(CALLBACK_MET(&CachedSegmentedFile::closeAfterWrite,*this));
}

file_size_t CachedSegmentedFile::getFileSize()
{
    ASSERT(cachedFile);
    return cachedFile->getFileSize();
}

num_segments_t CachedSegmentedFile::getNumSegments()
{
    return SegmentedFile::getNumSegments();
}

/////////////////////////////////////////////////////////////

void CachedSegmentedFile::closeAfterWrite(cometos_error_t result)
{
    ASSERT(cachedFile);
    // ASSERT(result == COMETOS_SUCCESS); // TODO ?
    cachedFile->close(closeCallback);
}

void CachedSegmentedFile::justifyCache()
{
    ASSERT(cachedFile);

    segment_size_t intendedSize = cachedFile->getMaxSegmentSize();

    if(intendedSize != cacheSize && cache != NULL) {
        delete[] cache;
        cache = NULL;
    }

    if(cache == NULL) {
        cache = new uint8_t[intendedSize];
        cacheDirty = true;
        cacheSize = intendedSize;
    }

    ASSERT(cache);
}

fsmReturnStatus CachedSegmentedFile::stateWriteIfNecessary(CachedSegmentedFileEvent& e)
{
    // ignore entry and exit events
    if(e.signal != CachedSegmentedFileEvent::RESULT_SIGNAL) {
        return FSM_IGNORED;
    }

    ASSERT(e.result == COMETOS_SUCCESS); // can only come from pass

    justifyCache();

    LOG_INFO("CachedSegmentedFile::stateWriteIfNecessary");

    bool necessary = false;

    switch(currentOperation) {
    case OPERATION::WRITE:
    case OPERATION::READ:
        // the segment to be used is different from the current segment which is dirty
        if(cachedSegment != externalSegment / (cachedFile->getMaxSegmentSize() / getMaxSegmentSize()) && cachedSegment >= 0 && cacheDirty) { 
            necessary = true;
        }
        break;
    case OPERATION::WRITE_CACHE:
        // there is a dirty cached segment
        if(cachedSegment >= 0 && cacheDirty) { 
            necessary = true;
        }
        break;
    default:
        ASSERT(1 == 2);
    }

    if(necessary) {
        LOG_INFO("CachedSegmentedFile::stateWriteIfNecessary::necessary");
        cachedFile->write(cache, cachedFile->getSegmentSize(cachedSegment), cachedSegment, dispatchCallback);
        cacheDirty = false;
    }
    else {
        LOG_INFO("CachedSegmentedFile::stateWriteIfNecessary::not_necessary");
        dispatchTask.load(COMETOS_SUCCESS);
        getScheduler().add(dispatchTask);
    }

    return transition(&CachedSegmentedFile::stateReadIfNecessary);
}

fsmReturnStatus CachedSegmentedFile::stateReadIfNecessary(CachedSegmentedFileEvent& e)
{
    // ignore entry and exit events
    if(e.signal != CachedSegmentedFileEvent::RESULT_SIGNAL) {
        return FSM_IGNORED;
    }

    ASSERT(cache);
    ASSERT(cachedFile);
    LOG_INFO("CachedSegmentedFile::stateReadIfNecessary result=" << e.result);

    if(e.result != COMETOS_SUCCESS) {
        dispatchTask.load(COMETOS_SUCCESS);
        getScheduler().add(dispatchTask);
        return transition(&CachedSegmentedFile::stateIdle);
    }

    bool necessary = false;

    switch(currentOperation) {
    case OPERATION::WRITE:
    case OPERATION::READ:
        if(cachedSegment != externalSegment / (cachedFile->getMaxSegmentSize() / getMaxSegmentSize())) {
            necessary = true;
        }
        break;
    case OPERATION::WRITE_CACHE:
        break;
    default:
        ASSERT(1 == 2);
    }

    // adapt to new segment
    cachedSegment = externalSegment / (cachedFile->getMaxSegmentSize() / getMaxSegmentSize());


    if(necessary) {
        cachedFile->read(cache, cachedFile->getSegmentSize(cachedSegment), cachedSegment, dispatchCallback);
        cacheDirty = false;
    }
    else {
        dispatchTask.load(COMETOS_SUCCESS);
        getScheduler().add(dispatchTask);
    }

    return transition(&CachedSegmentedFile::stateAdditionalIfNecessary);
}

fsmReturnStatus CachedSegmentedFile::stateAdditionalIfNecessary(CachedSegmentedFileEvent& e)
{
    // ignore entry and exit events
    if(e.signal != CachedSegmentedFileEvent::RESULT_SIGNAL) {
        return FSM_IGNORED;
    }

    ASSERT(cachedFile);
    LOG_INFO("CachedSegmentedFile::stateAdditionalIfNecessary result=" << e.result);

    if(e.result != COMETOS_SUCCESS) {
        dispatchTask.load(COMETOS_SUCCESS);
        getScheduler().add(dispatchTask);
        return transition(&CachedSegmentedFile::stateIdle);
    }

    // now the cache is synchronized

    if(currentOperation == OPERATION::WRITE || currentOperation == OPERATION::READ) {
        num_segments_t segmentInCache = externalSegment % (cachedFile->getMaxSegmentSize() / getMaxSegmentSize());

        if(currentOperation == OPERATION::WRITE) {
            memcpy(cache + segmentInCache*getMaxSegmentSize(), externalData, getSegmentSize(externalSegment));
            cacheDirty = true;
        }
        else {
            file_size_t offset = segmentInCache*getMaxSegmentSize();
            segment_size_t size = getSegmentSize(externalSegment);
            memcpy(externalData, cache + offset, size);
        }
    }

    dispatchTask.load(COMETOS_SUCCESS);
    getScheduler().add(dispatchTask);
    return transition(&CachedSegmentedFile::stateIdle);
}

fsmReturnStatus CachedSegmentedFile::stateIdle(CachedSegmentedFileEvent& e)
{
    // ignore entry and exit events
    if(e.signal != CachedSegmentedFileEvent::RESULT_SIGNAL) {
        return FSM_IGNORED;
    }

    LOG_INFO("CachedSegmentedFile::stateIdle");

    // final step
    if(externalCallback) {
        externalCallback(e.result);
    }

    return FSM_HANDLED;
}

void CachedSegmentedFile::dispatcher(cometos_error_t result)
{
    CachedSegmentedFileEvent e;
    e.signal = CachedSegmentedFileEvent::RESULT_SIGNAL;
    e.result = result;
    dispatch(e);
}
