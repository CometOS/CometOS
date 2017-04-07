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

#include "RAMSegmentedFile.h"
#include <map>
#include <vector>
#include "palId.h"
#include <mutex>

using namespace cometos;

static std::map<node_t,std::map<std::string,std::vector<uint8_t>*>> filesystem;
std::mutex filesystemLock;

RAMSegmentedFile::RAMSegmentedFile()
: SegmentedFile(), opened(false)
{
}

RAMSegmentedFile::~RAMSegmentedFile()
{
    // TODO remove vectors
    /*
    if(storage != NULL) {
        delete[] storage;
    }
    */
}

void RAMSegmentedFile::open(AirString& filename, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen)
{
    node_t id = palId_id();

    this->fileSize = fileSize;
    this->filename = filename;

    getArbiter()->assertRunning();

    ASSERT(!opened);

    std::string str(filename.getStr());
    filesystemLock.lock();
    if(filesystem[id].find(str) == filesystem[id].end()) {
        filesystem[id][str] = new std::vector<uint8_t>();
    }
    file = filesystem[id][str];

    if(getFileSize() != -1) {
        file->resize(getFileSize());
    }

    if(getFileSize() >= 0 && removeBeforeOpen) {
        memset(file->data(),0,getFileSize());
    }
    else {
        // -1 signals to reuse existing file
        this->fileSize = file->size(); // store actual file size
    }
    filesystemLock.unlock();

    opened = true;

    finish(finishedCallback, COMETOS_SUCCESS);
    return;
}

void RAMSegmentedFile::close(Callback<void(cometos_error_t result)> finishedCallback)
{
    getArbiter()->assertRunning();

    // reuse storage for next open
    opened = false;

    finish(finishedCallback, COMETOS_SUCCESS);
    return;
}

void RAMSegmentedFile::write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback)
{
	ASSERT(segment < getNumSegments());
    ASSERT(segment >= 0);
    //ASSERT(dataLength == getSegmentSize(segment));

    getArbiter()->assertRunning();
    //ASSERT(storage != NULL);

    file_size_t seek = segment*getMaxSegmentSize();
    segment_size_t segmentSize = getSegmentSize(segment);

    filesystemLock.lock();
    uint8_t* fdata = file->data();
    uint8_t* fdataseek = fdata+seek;
    memcpy(fdataseek,data,segmentSize);
    filesystemLock.unlock();

    finish(finishedCallback, COMETOS_SUCCESS);
    return;
}

void RAMSegmentedFile::flush(Callback<void(cometos_error_t result)> finishedCallback) {
    finishedCallback(COMETOS_SUCCESS);
}

void RAMSegmentedFile::read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback)
{
	ASSERT(segment < getNumSegments());
    ASSERT(segment >= 0);
    //ASSERT(dataLength == getSegmentSize(segment));

    getArbiter()->assertRunning();

   // ASSERT(storage != NULL);

    file_size_t seek = segment*getMaxSegmentSize();
    segment_size_t segmentSize = getSegmentSize(segment);

    filesystemLock.lock();
    uint8_t* fdata = file->data();
    uint8_t* fdataseek = fdata+seek;
    memcpy(data,fdataseek,segmentSize);
    filesystemLock.unlock();

    finish(finishedCallback, COMETOS_SUCCESS);
    return;
}

file_size_t RAMSegmentedFile::getFileSize()
{
    return fileSize;
}

bool RAMSegmentedFile::isOpen()
{
    return opened;
}

void RAMSegmentedFile::finish(Callback<void(cometos_error_t)> finishedCallback, cometos_error_t result)
{
    ASSERT(finishedCallback);
    finishTask.load(result);
    finishTask.setCallback(finishedCallback);
    getScheduler().add(finishTask);
}
