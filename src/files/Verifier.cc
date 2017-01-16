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
 * @author Stefan Unterschuetz (updateCRC)
 */

#include "Verifier.h"

Define_Module(cometos::Verifier);

using namespace cometos;

Verifier::Verifier(const char * service_name)
: Endpoint(service_name), replyAddr(0xffff), addition(false)
{
    fileRequest.setCallback(CALLBACK_MET(&Verifier::fileAccessGranted,*this));
}

void Verifier::initialize()
{
}

Arbiter* Verifier::getArbiter()
{
    return &arbiter;
}

/**
 * CRC calculation
 * (xmodem, polynom is 0x1021)
 */
uint16_t Verifier::updateCRC(uint16_t crc, const uint8_t* data, uint16_t length, bool addition)
{
    for(uint16_t j = 0; j < length; j++) {
        if(addition == true) {
	    crc = crc + ((uint16_t)data[j]);
	} else {
	    crc = crc ^ ((uint16_t)data[j] << 8);
            for (uint8_t i=0; i<8; i++) {
                if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
                else crc <<= 1;
            }
	}
    }
    return crc;
}

void Verifier::setFileWrapper(SegmentedFile* file)
{
    this->file = file;
}

void Verifier::useAddition(bool addition) {
   this->addition = addition;
}

void Verifier::getLocalFileProperties(cometos::AirString filename, Callback<void(FileProperties)> callback)
{
    ASSERT_RUNNING(&arbiter);

    this->localCallback = callback;
    prop.filenameCRC = updateCRC(0, (const uint8_t*)filename.getStr(), strlen(filename.getStr()), addition);
    prop.bytes = 0;
    prop.bytesNotNull = 0;
    prop.crc = 0;
    readSegment = -1;
    this->filename = filename;

    ASSERT(file);
    file->getArbiter()->request(&fileRequest);
}

void Verifier::fileAccessGranted()
{
    ASSERT_RUNNING(file->getArbiter());
    file->setMaxSegmentSize(FILE_SEGMENT_SIZE);
    file->open(filename, -1, CALLBACK_MET(&Verifier::read,*this));
}

void Verifier::read(cometos_error_t result)
{
    if(result != COMETOS_SUCCESS) {
        file->close(CALLBACK_MET(&Verifier::final,*this));
        return;
    }

    if(readSegment == -1) {
        // first iteration
        prop.bytes = file->getFileSize();
        prop.bytesNotNull = 0;
        prop.crc = 0;
    }
    else {
        // perform operation on read data
        uint8_t len = file->getSegmentSize(readSegment);

        prop.crc = updateCRC(prop.crc, buf, len, addition);

        for(uint8_t j = 0; j < len; j++) {
            if(buf[j] != 0) {
                prop.bytesNotNull++;
            }
        }
    }

    readSegment++;

    // check if enough data read
    if(readSegment < file->getNumSegments()) {
        segment_size_t len = file->getSegmentSize(readSegment);
        file->read(buf, len, readSegment, CALLBACK_MET(&Verifier::read,*this));
    }
    else {
        prop.result = COMETOS_SUCCESS;
        file->close(CALLBACK_MET(&Verifier::final,*this));
    }
}

void Verifier::final(cometos_error_t result)
{
    if(result != COMETOS_SUCCESS) {
        prop.result = result;
    }

    palExec_atomicBegin();
    Callback<void(FileProperties)> tmpcb;
    file->getArbiter()->release();
    tmpcb = localCallback;
    localCallback = EMPTY_CALLBACK();
    palExec_atomicEnd();

    ASSERT(tmpcb);
    tmpcb(prop);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void Verifier::getRemoteFileProperties(node_t remote, AirString filename, Callback<void(node_t, FileProperties)> callback)
{
    ENTER_METHOD_SILENT();
    ASSERT_RUNNING(&arbiter);

    palExec_atomicBegin();
    this->remoteCallback = callback;
    palExec_atomicEnd();

    AirframePtr frame = make_checked<Airframe>();
    (*frame) << filename;
    (*frame) << (uint8_t)CMD::REQUEST;

    DataRequest* req = new DataRequest(remote, frame);
    Endpoint::sendRequest(req);
}

void Verifier::handleIndication(DataIndication* msg)
{
    CMD cmd;
    msg->getAirframe() >> (uint8_t&)cmd;

    if(cmd == CMD::REQUEST) {
        msg->setBoundedDelegate(UnboundedDelegate(&Verifier::msgRequestGranted), this);
        getArbiter()->request(msg->getEvent());
    }
    else if(cmd == CMD::RESPONSE) {
        FileProperties remoteProperties;
        msg->getAirframe() >> remoteProperties;

        palExec_atomicBegin();
        auto tmpcb = remoteCallback;
        remoteCallback = EMPTY_CALLBACK();
        palExec_atomicEnd();
        ASSERT(tmpcb);
        tmpcb(msg->src,remoteProperties);
        delete(msg);
    }
    else {
        ASSERT(false);
        delete(msg);
    }
}

void Verifier::msgRequestGranted(DataIndication* msg)
{
    cometos::AirString filename;
    msg->getAirframe() >> filename;
    replyAddr = msg->src;
    getLocalFileProperties(filename, CALLBACK_MET(&Verifier::sendResult,*this));
    delete(msg);
}

void Verifier::sendResult(FileProperties properties)
{
    getArbiter()->release();

    if(replyAddr != 0xffff) {
        AirframePtr frame = make_checked<Airframe>();
        (*frame) << properties;
        (*frame) << (uint8_t)CMD::RESPONSE;

        DataRequest* req = new DataRequest(replyAddr, frame);
        Endpoint::sendRequest(req);
    }
}

FileProperties Verifier::getLastFileProperties()
{
    return prop;
}

