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

#include "DelugeInfoFile.h"

using namespace cometos;

DelugeInfoFile::DelugeInfoFile() : mFilename(DELUGE_INFO_FILE), pFile(SegFileFactory::CreateInstance()) {

}

DelugeInfoFile::~DelugeInfoFile() {
    if (this->pFile) {
        delete this->pFile;
    }
}

void DelugeInfoFile::getInfo(Callback<void(cometos_error_t,DelugeInfo*)> callback) {
#ifdef DELUGE_OUTPUT
    getCout() << __PRETTY_FUNCTION__ << ": Reading info file" << endl;
#endif

    // Store callback
    this->mDoneCallback = callback;

    // Open file
    this->mSegmentIndex = 0;
    this->mBufferByteIndex = 0;
    this->pFile->setMaxSegmentSize(DELUGE_INFOFILE_SEGMENT_SIZE);
    ASSERT(this->pFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    this->pFile->open(this->mFilename, DELUGE_MAX_INFOFILE_SIZE, CALLBACK_MET(&DelugeInfoFile::readSegment, *this));
}

void DelugeInfoFile::writeInfo(DelugeInfo *pInfo, Callback<void(cometos_error_t,DelugeInfo*)> callback) {
#ifdef DELUGE_OUTPUT
    getCout() << __PRETTY_FUNCTION__ << ": Writing info file" << endl;
#endif

    this->mDoneCallback = callback;
    this->pInfo = pInfo;

    // Copy info into buffer
    memset(mBuffer, 0x12, DELUGE_MAX_INFOFILE_SIZE);
    uint16_t byteIndex = 0;
    uint16_t version = pInfo->getVersion();
    memcpy(&mBuffer[byteIndex], &version, 2);
    byteIndex += 2;
    uint8_t numberOfPages = pInfo->getNumberOfPages();
    memcpy(&mBuffer[byteIndex], &numberOfPages, 1);
    byteIndex += 1;
    file_size_t fileSize = pInfo->getFileSize();
    memcpy(&mBuffer[byteIndex], &fileSize, 4);
    byteIndex += 4;
    memcpy(&mBuffer[byteIndex], pInfo->getPageAge(), pInfo->getAgeVectorSize());
    byteIndex += pInfo->getAgeVectorSize();
    memcpy(&mBuffer[byteIndex], pInfo->getPageCRC(), pInfo->getCRCVectorSize());
    byteIndex += pInfo->getCRCVectorSize();
    memcpy(&mBuffer[byteIndex], pInfo->getPageComplete(), pInfo->getPageCompleteSize());
    byteIndex += pInfo->getPageCompleteSize();

    // Open file
    this->mSegmentIndex = 0;
    this->mBufferByteIndex = 0;
    this->pFile->setMaxSegmentSize(DELUGE_INFOFILE_SEGMENT_SIZE);
    ASSERT(this->pFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    this->pFile->open(this->mFilename, DELUGE_MAX_INFOFILE_SIZE, CALLBACK_MET(&DelugeInfoFile::writeSegment, *this));
}

void DelugeInfoFile::readSegment(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Check if all segments are read
    if (this->mSegmentIndex >= this->pFile->getNumSegments())
        return this->parseBuffer();

    // Read next segment
    this->pFile->read(&mBuffer[this->mBufferByteIndex], this->pFile->getSegmentSize(this->mSegmentIndex), this->mSegmentIndex, CALLBACK_MET(&DelugeInfoFile::readSegment, *this));
    this->mBufferByteIndex += this->pFile->getSegmentSize(this->mSegmentIndex);
    this->mSegmentIndex++;
}

void DelugeInfoFile::writeSegment(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Check if all segments are written
    if (this->mSegmentIndex >= this->pFile->getNumSegments())
        return this->pFile->close(CALLBACK_MET(&DelugeInfoFile::finalize, *this));

    // Write next segment
    this->pFile->write(&mBuffer[this->mBufferByteIndex], this->pFile->getSegmentSize(this->mSegmentIndex), this->mSegmentIndex, CALLBACK_MET(&DelugeInfoFile::writeSegment, *this));
    this->mBufferByteIndex += this->pFile->getSegmentSize(this->mSegmentIndex);
    this->mSegmentIndex++;
}

void DelugeInfoFile::finalize(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
    this->pFile->getArbiter()->release();

    // Clear buffer
#ifdef DELUGE_OUTPUT
    getCout() << __PRETTY_FUNCTION__ << ": Info file processed for version=" << this->pInfo->getVersion() << endl;
#endif

    // Call callback
    this->mDoneCallback(COMETOS_SUCCESS, this->pInfo);

    // Clear info file
    this->pInfo = nullptr;
}

void DelugeInfoFile::parseBuffer() {
    uint16_t byteIndex = 0;

    // read version number
    uint16_t versionNumber = 0;
    memcpy(&versionNumber, &mBuffer[byteIndex], 2);
    byteIndex += 2;

    // read number of pages
    uint8_t numberOfPages = 0;
    memcpy(&numberOfPages, &mBuffer[byteIndex], 1);
    byteIndex += 1;

    file_size_t fileSize = 0;
    memcpy(&fileSize, &mBuffer[byteIndex], 4);
    byteIndex += 4;

    // read age vector
    uint16_t avSize = DelugeInfo::GetAgeVectorSize(numberOfPages);
    uint8_t* pAgeVector = new uint8_t[avSize];
    memcpy(pAgeVector, &mBuffer[byteIndex], avSize);
    byteIndex += avSize;

    // read crc vector
    uint16_t* pCRCVector = new uint16_t[numberOfPages];
    memcpy(pCRCVector, &mBuffer[byteIndex], numberOfPages*2);
    byteIndex += numberOfPages*2;

    // read page complete vector
    uint16_t pcvSize = DelugeInfo::GetPageCompleteSize(numberOfPages);
    uint8_t* pPageComplete = new uint8_t[pcvSize];
    memcpy(pPageComplete, &mBuffer[byteIndex], pcvSize);
    byteIndex += pcvSize;

    // Store info
    this->pInfo = new DelugeInfo(versionNumber, numberOfPages, fileSize, pAgeVector, pCRCVector, pPageComplete);

    // Check if we have to write last segment in order to reserve space
    if (versionNumber == 0) {
        memset(mBuffer, 0x12, DELUGE_INFOFILE_SEGMENT_SIZE);
        return this->pFile->write(mBuffer, this->pFile->getSegmentSize(this->pFile->getNumSegments()-1), this->pFile->getNumSegments()-1, CALLBACK_MET(&DelugeInfoFile::lastSegmentWritten, *this));
    }

    this->pFile->close(CALLBACK_MET(&DelugeInfoFile::finalize, *this));
}

void DelugeInfoFile::lastSegmentWritten(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Close file
    this->pFile->close(CALLBACK_MET(&DelugeInfoFile::finalize, *this));
}


















