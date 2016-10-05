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

#include "SimpleFileTransfer.h"
#include "Verifier.h"
#include "DataIndication.h"
#include "logging.h"

Define_Module(cometos::SimpleFileTransfer);

using namespace cometos;

#define INTERVAL_MS 100

SimpleFileTransfer::SimpleFileTransfer(const char *name)
: Endpoint(name), intervalMS(INTERVAL_MS), initiator(false), persistentFile(NULL), data()
{
    dataReadTask.setCallback(CALLBACK_MET(&SimpleFileTransfer::dataReadStart,*this));
}

void SimpleFileTransfer::setIntervalMS(int intervalMS)
{
    this->intervalMS = intervalMS;
}

void SimpleFileTransfer::setFileWrapper(SegmentedFile* file)
{
    this->persistentFile = file;
    ASSERT(cacheFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own cacheFile
    this->cacheFile.setCachedFile(persistentFile);
    persistentFile->setMaxSegmentSize(PERSISTENT_SEGMENT_SIZE);
    cacheFile.setMaxSegmentSize(PACKET_SEGMENT_SIZE);
    cacheFile.getArbiter()->release();
}

Arbiter* SimpleFileTransfer::getArbiter()
{
    return &arbiter;
}

void SimpleFileTransfer::run(node_t receiver, cometos::AirString localFilename, cometos::AirString remoteFilename, Callback<void(cometos_error_t,node_t)> finishedCallback)
{
    ENTER_METHOD_SILENT();
    arbiter.assertRunning();

    this->receiver = receiver;
    this->localFilename = localFilename;
    this->remoteFilename = remoteFilename;
    this->filenameCRC = Verifier::updateCRC(0, (const uint8_t*)localFilename.getStr(), strlen(localFilename.getStr()));
    this->finishedCallback = finishedCallback;
    this->currentSegment = 0;
    this->initiator = true;

    ASSERT(cacheFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own cacheFile
    cacheFile.open(localFilename, -1, CALLBACK_MET(&SimpleFileTransfer::localFileOpened,*this));
}

void SimpleFileTransfer::localFileOpened(cometos_error_t result)
{
    arbiter.assertRunning();
    cacheFile.getArbiter()->release();

    if(result != COMETOS_SUCCESS) {
        close(result);
        return;
    }

    // send open message to the client(s)
    Airframe* frame = new Airframe();
    (*frame) << remoteFilename;
getCout() << "Send " << remoteFilename.getStr() << endl;
    (*frame) << filenameCRC;
    (*frame) << (file_size_t)cacheFile.getFileSize();
    (*frame) << (uint8_t)CMD::OPEN;

    DataRequest* req = new DataRequest(receiver, frame, createCallback(&SimpleFileTransfer::handleResponse));
    Endpoint::sendRequest(req);
}

void SimpleFileTransfer::handleResponse(DataResponse* resp) {
    arbiter.assertRunning();

    if(!resp->success) {
        close(COMETOS_ERROR_FAIL);
        delete(resp);
        return;
    }

    delete(resp);

    if(currentSegment >= cacheFile.getNumSegments()) {
        close(COMETOS_SUCCESS);
        return;
    }

    getScheduler().add(dataReadTask,intervalMS);
}

void SimpleFileTransfer::dataReadStart() {
    // read from file (or the cache)
    ASSERT(cacheFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own cacheFile
    cacheFile.read(data.getBuffer(), cacheFile.getSegmentSize(currentSegment), currentSegment, CALLBACK_MET(&SimpleFileTransfer::dataReadFinished,*this));
}

void SimpleFileTransfer::dataReadFinished(cometos_error_t result) {
    arbiter.assertRunning();
    cacheFile.getArbiter()->release();

    if(result != COMETOS_SUCCESS) {
        close(result);
        return;
    }

    data.setSize(cacheFile.getSegmentSize(currentSegment));

    // send payload message to the client(s)
    Airframe* frame = new Airframe();
    (*frame) << data;
    (*frame) << currentSegment;
    (*frame) << filenameCRC;
    (*frame) << (uint8_t)CMD::SEGMENT;

    if(currentSegment % 10 == 0) {
        cometos::getCout() << "s" << currentSegment << " ";
    }

    currentSegment++;

    DataRequest* req = new DataRequest(receiver, frame, createCallback(&SimpleFileTransfer::handleResponse));
    Endpoint::sendRequest(req);
}

void SimpleFileTransfer::close(cometos_error_t result)
{
    arbiter.assertRunning();
    finalResult = result;

    if(initiator) {
        // send close message to the client(s)
        Airframe* frame = new Airframe();
        (*frame) << (uint8_t)CMD::CLOSE;

        DataRequest* req = new DataRequest(receiver, frame);
        Endpoint::sendRequest(req);
    }

    // close the file
    ASSERT(cacheFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own cacheFile
    cacheFile.close(CALLBACK_MET(&SimpleFileTransfer::finalize,*this));
}

void SimpleFileTransfer::finalize(cometos_error_t result)
{
    arbiter.assertRunning();
    cacheFile.getArbiter()->release();

    if(finalResult != COMETOS_SUCCESS) {
        result = finalResult;
    }

    auto tmpCb = finishedCallback;
    finishedCallback = EMPTY_CALLBACK();

    // from here on, use only local variables!
    
    ASSERT(tmpCb);
    tmpCb(result,this->receiver);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void SimpleFileTransfer::fileOpenedFromRemote(cometos_error_t result)
{
    cacheFile.getArbiter()->release();

    if(result != COMETOS_SUCCESS) {
        getCout() << "Opening file failed" << endl;
        arbiter.release();
    }
}

void SimpleFileTransfer::handleIndication(DataIndication* msg)
{
    ASSERT(persistentFile);

    CMD cmd;
    msg->getAirframe() >> (uint8_t&)cmd;

    if(cmd == CMD::OPEN) {
        LOG_DEBUG("CMD::OPEN");
        palExec_atomicBegin();
        cometos_error_t result = arbiter.requestImmediately();
        if(result != COMETOS_SUCCESS) {
            getCout() << "Another file transfer still in progress" << endl;
            delete(msg);
            palExec_atomicEnd();
            return;
        }
        palExec_atomicEnd();

        finishedCallback = arbiter.generateReleaseCallback<cometos_error_t,node_t>();

        // extract the file information
        file_size_t size;
        msg->getAirframe() >> size;
        msg->getAirframe() >> filenameCRC;
        msg->getAirframe() >> localFilename;

        LOG_DEBUG("cacheFile.open");

        // open the file
        ASSERT(cacheFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own cacheFile
        cacheFile.open(localFilename, size, CALLBACK_MET(&SimpleFileTransfer::fileOpenedFromRemote,*this), true);
        delete(msg);
    }
    else if(!arbiter.isRunning() || !cacheFile.isOpen()) {
        LOG_ERROR("drop " << arbiter.isRunning() << " " << cacheFile.isOpen());
		//ASSERT(false); // should not happen in normal operation
        delete(msg); // drop the message
        return;
    }
    else if(cmd == CMD::CLOSE) {
        LOG_DEBUG("CMD::CLOSE");
        delete(msg);
        close(COMETOS_SUCCESS);
        return;
    }
    else if(cmd == CMD::SEGMENT && arbiter.isRunning()) {
        //LOG_DEBUG("CMD::SEGMENT");
        // extract the segment information
        uint16_t segmentFilenameCRC;
        msg->getAirframe() >> segmentFilenameCRC;

        if(filenameCRC != segmentFilenameCRC) {
            // ignore the message
            delete(msg);
            return;
        }

        // write to file (or the cache)
        num_segments_t segment;
        msg->getAirframe() >> segment;
        msg->getAirframe() >> data;

        if(segment % 10 == 0) {
            cometos::getCout() << "r" << segment << " ";
        }

        ASSERT(cacheFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own cacheFile
        cacheFile.write(data.getBuffer(), cacheFile.getSegmentSize(segment), segment, cacheFile.getArbiter()->generateReleaseCallback<unsigned char>());
        delete(msg);
    }
    else {
        LOG_ERROR("drop");
        ASSERT(false); // should not happen in normal operation
        delete(msg);
    }
}
