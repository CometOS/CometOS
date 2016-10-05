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

#include "CFSSegmentedFile.h"
#include "CFSArbiter.h"
#include "cfs.h"
#include "palWdt.h"
#include "cfs/cfs-coffee.h"

using namespace cometos;

CFSSegmentedFile::CFSSegmentedFile()
: SegmentedFile(), fd(-1)
{
    writeSegmentTask.setCallback(CALLBACK_MET(&CFSSegmentedFile::writeSegment, *this));
    readSegmentTask.setCallback(CALLBACK_MET(&CFSSegmentedFile::readSegment, *this));
    openFileTask.setCallback(CALLBACK_MET(&CFSSegmentedFile::openFile, *this));
    closeFileTask.setCallback(CALLBACK_MET(&CFSSegmentedFile::closeFile, *this));
    flushTask.setCallback(CALLBACK_MET(&CFSSegmentedFile::flushFile, *this));
}

void CFSSegmentedFile::open(AirString& filename, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen)
{
    getArbiter()->assertRunning();
    this->fileSize = fileSize;
    this->finishedCallback = finishedCallback;
    openFileTask.load(filename,removeBeforeOpen);
    request.setCallback(&openFileTask);
    getCFSArbiter()->request(&request);
}

void CFSSegmentedFile::close(Callback<void(cometos_error_t result)> finishedCallback)
{
    getArbiter()->assertRunning();
    this->finishedCallback = finishedCallback;
    request.setCallback(&closeFileTask);
    getCFSArbiter()->request(&request);
}

void CFSSegmentedFile::write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback)
{
    ASSERT(segment < getNumSegments());
    ASSERT(segment >= 0);
    ASSERT(dataLength == getSegmentSize(segment));

    getArbiter()->assertRunning();
    this->finishedCallback = finishedCallback;
    writeSegmentTask.load(data,segment);
    request.setCallback(&writeSegmentTask);
    getCFSArbiter()->request(&request);
}

void CFSSegmentedFile::flush(Callback<void(cometos_error_t result)> finishedCallback)
{
    getArbiter()->assertRunning();
    this->finishedCallback = finishedCallback;
    request.setCallback(&flushTask);
    getCFSArbiter()->request(&request);
}

void CFSSegmentedFile::read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback)
{
    ASSERT(segment < getNumSegments());
    ASSERT(segment >= 0);
    ASSERT(dataLength == getSegmentSize(segment));

    getArbiter()->assertRunning();
    this->finishedCallback = finishedCallback;
    readSegmentTask.load(data,segment);
    request.setCallback(&readSegmentTask);
    getCFSArbiter()->request(&request);
}

void CFSSegmentedFile::writeSegment(uint8_t* data, num_segments_t segment)
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    file_size_t seek = segment*(file_size_t)getMaxSegmentSize();
    file_size_t ret = cfs_seek(fd, seek, CFS_SEEK_SET);
    if(ret != seek) {
        LOG_INFO("cfs_seek returned " << ret << " instead of " << seek);
        finish(COMETOS_ERROR_FAIL);
        return;
    }

    segment_size_t segmentSize = getSegmentSize(segment);
    ret = cfs_write(fd, data, segmentSize);
    if(ret != segmentSize) {
        LOG_INFO("cfs_write returned " << ret << " instead of " << segmentSize);
        finish(COMETOS_ERROR_FAIL);
        return;
    }

    finish(COMETOS_SUCCESS);
    return;
}

void CFSSegmentedFile::readSegment(uint8_t* data, segment_size_t segment)
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    file_size_t seek = segment*(file_size_t)getMaxSegmentSize();
    file_size_t ret = cfs_seek(fd, seek, CFS_SEEK_SET);
    if(ret != seek) {
        getCout() << "cfs_seek returned " << ret << " instead of " << seek;
        ASSERT(false);
        finish(COMETOS_ERROR_FAIL);
        return;
    }

    segment_size_t segmentSize = getSegmentSize(segment);
    ret = cfs_read(fd, data, segmentSize);
    if(ret != segmentSize) {
        // fill up the data if not existent in file, yet
        file_size_t i = ret;
        if(i < 0) {
            i = 0;
        }

        for(; i < segmentSize; i++)
        {
            data[i] = 0;
        }
    }

    finish(COMETOS_SUCCESS);
    return;
}

void CFSSegmentedFile::openFile(AirString filename, bool removeBeforeOpen)
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    // file still open?
    ASSERT(fd < 0);

    opened = false;

    if(removeBeforeOpen) {
        cfs_remove(filename.getStr());
    }

    file_size_t reserveSize = getFileSize();
    if(reserveSize == -1) {
        reserveSize = 0;
    }

    if(cfs_reserve(filename.getStr(), reserveSize) == 0) {
        // new file
        if(getFileSize() == -1)
        {getCout() << "here is failure0" << endl;
            // new but automatic size, not possible!
            finish(COMETOS_ERROR_FAIL);
            return;
        }

        fd = cfs_open(filename.getStr(), CFS_READ | CFS_WRITE);

        if(fd < 0) {getCout() << "here is failure" << endl;
            finish(COMETOS_ERROR_FAIL);
            return;
        }
    }
    else {
        // probably this file already exists
        fd = cfs_open(filename.getStr(), CFS_READ | CFS_WRITE | CFS_APPEND);

        if(fd < 0) {getCout() << "here is failure2" << endl;
            finish(COMETOS_ERROR_FAIL);
            return;
        }

        file_size_t fileSize = cfs_seek(fd, 0, CFS_SEEK_END);
        if(fileSize != getFileSize() && getFileSize() != -1) {
            getCout() << "here is failure3 --> " << fileSize << " / " << getFileSize() << endl;
            finish(COMETOS_ERROR_FAIL);
            return;
        }

        this->fileSize = fileSize; // store actual file size

        cfs_seek(fd, 0, CFS_SEEK_SET);
    }

    opened = true;

    cfs_coffee_configure_log(filename.getStr(), 63000, 90); //TODO

    finish(COMETOS_SUCCESS);
    return;
}

void CFSSegmentedFile::closeFile()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    if(fd >= 0) {
        cfs_close(fd);
        fd = -1;
    }

    opened = false;

    finish(COMETOS_SUCCESS);
    return;
}

void CFSSegmentedFile::flushFile()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    palWdt_pause();
    num_segments_t res = cfs_flush(fd);
    palWdt_resume();

    if(res < 0) {
        LOG_INFO("Failed to flush the written data");
        finish(COMETOS_ERROR_FAIL);
        return;
    }

    finish(COMETOS_SUCCESS);
    return;
}

file_size_t CFSSegmentedFile::getFileSize()
{
    return fileSize;
}

bool CFSSegmentedFile::isOpen()
{
    return opened;
}

void CFSSegmentedFile::finish(cometos_error_t result)
{
    palExec_atomicBegin();
    Callback<void(cometos_error_t)> tmpcb;
    getCFSArbiter()->release();
    tmpcb = finishedCallback;
    finishedCallback = EMPTY_CALLBACK();
    palExec_atomicEnd();

    ASSERT(tmpcb);
    tmpcb(result);
}
